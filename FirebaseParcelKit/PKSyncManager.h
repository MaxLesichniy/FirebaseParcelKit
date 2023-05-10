//
//  PKSyncManager.h
//  ParcelKit
//
//  Copyright (c) 2013 Overcommitted, LLC. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <FirebaseDatabase/FirebaseDatabase.h>

@class PKSyncManager;

@protocol PKSyncManagerDelegate <NSObject>
@optional
- (void)syncManager:(PKSyncManager *)syncManager managedObject:(NSManagedObject *)managedObject insertValidationFailed:(NSError *)error inManagedObjectContext:(NSManagedObjectContext *)managedObjectContext;
- (void)managedObjectWasSyncedFromFirebase:(NSManagedObject *)managedObject syncManager:(PKSyncManager *)syncManager;
- (void)managedObjectWasSyncedToFirebase:(NSManagedObject *)managedObject syncManager:(PKSyncManager *)syncManager;
- (void)willDeleteManagedObjectFromFirebase:(NSManagedObject *)managedObject syncManager:(PKSyncManager *)syncManager;
- (void)managedObject:(NSManagedObject *)managedObject invalidAttribute:(NSString*)propertyName value:(id)value expected:(Class)expectedClass;
- (BOOL)isRecordSyncable:(NSManagedObject *)managedObject;
- (NSDictionary*)syncedPropertiesDictionary:(NSDictionary*)propertiesByName forManagedObject:(NSManagedObject*)managedObject reference:(FIRDatabaseReference*)reference;
- (NSDictionary*)syncManager:(PKSyncManager*)syncManager transformRemoteData:(NSDictionary*)remoteData forEntityName:(NSString*)entityName reference:(FIRDatabaseReference*)reference;
- (NSArray*)syncedPropertyNamesForManagedObject:(NSManagedObject*)managedObject;
- (FIRDatabaseReference*)containerForObject:(NSManagedObject *)managedObject syncManager:(PKSyncManager *)syncManager;
- (BOOL)syncManager:(PKSyncManager *)syncManager hasManagedObjectChanged:(NSManagedObject *)managedObject;
@end

extern NSString * const PKDefaultSyncIDAttributeName;
extern NSString * const PKDefaultIsSyncedAttributeName;

/**
 Notification that is posted when the sync status changes.
 
 The userInfo of the notification will contain the PKSyncStatus in `PKSyncManagerFirebaseStatusKey`
 */
extern NSString * const PKSyncManagerFirebaseStatusDidChangeNotification;
extern NSString * const PKSyncManagerFirebaseStatusKey;

/**
 Notification that is posted when Firebase has incoming changes.

 The userInfo of the notification will contain the Firebase change NSDictionary in `PKSyncManagerFirebaseIncomingChangesKey`
 */
extern NSString * const PKSyncManagerFirebaseIncomingChangesNotification;
extern NSString * const PKSyncManagerFirebaseIncomingChangesKey;

/**
 Firebase sync status
 */
@interface PKSyncStatus : NSObject
/** True if data is currently being received by the sync engine */
@property (nonatomic) BOOL downloading;
/** True if data is currently being transmitted to Firebase */
@property (nonatomic) BOOL uploading;
@property (nonatomic) NSUInteger uploadedRecords;
@property (nonatomic) NSUInteger totalRecordsToUpload;
/** N.B. This has not been implemented yet */
@property (nonatomic) NSUInteger downloadedRecords;
@property (nonatomic) NSUInteger totalRecordsToDownload;
@property (nonatomic, retain) NSError* lastError;
@property (nonatomic, retain) NSString* lastErrorSummary;
@end

/** 
 The sync manager is responsible for listening to changes from a
 Core Data NSManagedObjectContext and a Firebase database and syncing the changes between them.
 */
@interface PKSyncManager : NSObject

/** 
 The Core Data managed object context to listen for changes from.
 */
@property (nonatomic, strong, readonly) NSManagedObjectContext *managedObjectContext;

/** The Firebase Database to read and write to. */
@property (nonatomic, strong, readonly) FIRDatabaseReference *databaseRoot;

/**
 The Core Data entity attribute name to use for keeping managed objects in sync.
 
 The default value is “syncID”.
*/
@property (nonatomic, copy) NSString *syncIDAttributeName;

/**
 The Core Data entity attribute name to use for keeping managed objects in sync.
 
 The default value is “isSynced”.
 */
@property (nonatomic, copy) NSString *isSyncedAttributeName;

/**
 The key name in Firebase to store the local device ID of the device that last updated Firebase.
 
 The default value is "lastDeviceID".
 */
@property (nonatomic, copy) NSString *lastDeviceIdAttributeName;

/**
 The key name in Firebase to store the last modified timestamp in Firebase.
 
 The default value is "remoteSyncTimestamp".
 */
@property (nonatomic, copy) NSString *remoteSyncTimestampAttributeName;


/**
 Delegate that can handle various edge cases in an app-specific manner.
*/
@property (nonatomic, weak) id<PKSyncManagerDelegate> delegate;

/**
 A string that uniquely identifies this device vs other devices
 */
@property (nonatomic, copy) NSString* localDeviceId;

/**
 Current status and progress of the syncing
 */
@property (atomic, strong) PKSyncStatus* currentSyncStatus;

/**
 Returns a random string suitable for using as a sync identifer.
 @return A random string suitable for using as a sync identifer.
 */
+ (NSString *)syncID;

/** @name Creating and Configuring a Sync Manager */

/**
 The designated initializer used to specify the Core Data managed object context and the Dropbox data store that should be synchronized.
 
 @param managedObjectContext The Core Data managed object context the sync manager should listen for changes from.
 @param queue The queue that all callbacks will be executed on
 @return A newly initialized `PKSyncManager` object.
 */
- (instancetype)initWithManagedObjectContext:(NSManagedObjectContext *)managedObjectContext queue:(dispatch_queue_t)queue;

/**
 Map multiple Core Data entity names to their corresponding Dropbox data store table name. Replaces all other existing relationships that may have been previously set.
 @param keyedTables Dictionary of key/value pairs where the key is the Core Data entity name and the value is the corresponding Firebase database table name.
 */
- (void)setTablesForEntityNamesWithDictionary:(NSDictionary *)keyedTables;

/**
 Maps a single Core Data entity name to the corresponding Dropbox data store table name.
 
 Replaces any existing relationship for the given entity name that may have been previously set.
 Will raise an NSInternalInconsistencyException if the entity does not contain a valid sync attribute.
 @param tableID The Firebase database tableID that the entity name should be mapped to.
 @param entityName The Core Data entity name that should map to the given tableID.
 */
- (void)setTable:(NSString *)tableID forEntityName:(NSString *)entityName;

/**
 Removes the Core Data <-> Firebase mapping for the given entity name.
 @param entityName The Core Data entity name that should no longer be mapped to Firebase.
 */
- (void)removeTableForEntityName:(NSString *)entityName;

/** @name Accessing Entity Names and Tables */

/** 
 Returns a dictionary of tables mapped to their corresponding entity names.
 @return A dictionary of tables mapped to their corresponding entity names.
 */
- (NSDictionary *)tablesByEntityName;

/**
 Returns an array of currently mapped Firebase database tableIDs.
 @return An array of currently mapped Firebase database tableIDs.
 */
- (NSArray *)tableIDs;

/**
 Returns an array of currently mapped Core Data entity names.
 @return An array of currently mapped Core Data entity names.
 */
- (NSArray *)entityNames;

/**
 Returns the tableID associated with a given entity name.
 @param entityName The entity name for which to return the corresponding tableID.
 @return The tableID associated with entityName, or nil if no tableID is associated with entityName.
 */
- (NSString *)tableForEntityName:(NSString *)entityName;

/** @name Observing Changes */

/**
 Returns whether or not the sync manager is currently observing changes.
 
 The default value is `NO`.
 @return `NO` if the sync manager is not observing changes, `YES` if it is.
 */
- (BOOL)isObserving;

/**
 Returns whether or not the sync manager is currently observing changes to the specified container.
 
 The default value is `NO`.
 @return `NO` if the sync manager is not observing changes to the given container, `YES` if it is.
 */
- (BOOL)isObservingContainer:(FIRDatabaseReference*)container;

/**
 Starts observing changes to the Core Data managed object context and the Firebase database.
 @param container The containing database reference where we should look for tables to observe
 */
- (void)startObservingContainer:(FIRDatabaseReference*)container;

/**
 Stops observing changes from the Core Data managed object context and the Firebase database.
 */
- (void)stopObserving;

/**
 Convert a timestamp into a string
 @param date The date to convert into a timestamp
 */
- (NSString *)TTTISO8601TimestampFromDate:(NSDate *)date;

/**
 Convert a string into a timestamp
 @param timestamp The timestamp to convert into a date
 */
- (NSDate *)TTTDateFromISO8601Timestamp:(NSString *)timestamp;

/**
 Log an error to do with syncing
 @param error The error
 @param errorSummary Human readable summary
 */
- (void)setLastError:(NSError*)error summary:(NSString*)errorSummary;

@end
