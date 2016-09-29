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
- (void)managedObject:(NSManagedObject *)managedObject invalidAttribute:(NSString*)propertyName value:(id)value expected:(Class)expectedClass;
- (BOOL)isRecordSyncable:(NSManagedObject *)managedObject;
@end

extern NSString * const PKDefaultSyncAttributeName;
extern NSString * const PKDefaultIsSyncedAttributeName;

/**
 REDO: Notification that is posted when the DBDatastoreStatus changes.
 
 REDO: The userInfo of the notification will contain the DBDatastoreStatus in `PKSyncManagerDatastoreStatusKey`
 */
extern NSString * const PKSyncManagerCouchbaseStatusDidChangeNotification;
extern NSString * const PKSyncManagerCouchbaseStatusKey;

/**
 Notification that is posted when Couchbase Lite has incoming changes.

 REDO: The userInfo of the notification will contain the DBDatastore change NSDictionary in `PKSyncManagerDatastoreStatusKey`
 */
extern NSString * const PKSyncManagerCouchbaseIncomingChangesNotification;
extern NSString * const PKSyncManagerCouchbaseIncomingChangesKey;

/**
 Notification that is posted when the sync is ok.
 
 The userInfo of the notification will contain the last sync date in `PKSyncManagerCouchbaseLastSyncDateKey`
 */
extern NSString * const PKSyncManagerCouchbaseLastSyncDateNotification;
extern NSString * const PKSyncManagerCouchbaseLastSyncDateKey;


/** 
 The sync manager is responsible for listening to changes from a
 Core Data NSManagedObjectContext and a Couchbase Lite CBLDatabase and syncing the changes between them.
 */
@interface PKSyncManager : NSObject

/** 
 The Core Data managed object context to listen for changes from.
 */
@property (nonatomic, strong, readonly) NSManagedObjectContext *managedObjectContext;

/** The Couchbase Database to read and write to. */
@property (nonatomic, strong, readonly) FIRDatabaseReference *databaseRoot;

/**
 The Core Data entity attribute name to use for keeping managed objects in sync.
 
 The default value is “syncID”.
*/
@property (nonatomic, copy) NSString *syncAttributeName;

/**
 The Core Data entity attribute name to use for keeping managed objects in sync.
 
 The default value is “isSynced”.
 */
@property (nonatomic, copy) NSString *isSyncedAttributeName;


/**
 The number of Core Data managed objects to sync with the DBDatastore at a time.
 
 The DBDatastore has a 2 MiB delta size limit so changes in the managed object context
 must be batched to remain below this limit.
 
 The default value is “20”. (2048 KiB max delta size / 100 KiB max record size)
*/
@property (nonatomic) NSUInteger syncBatchSize;

/**
 Delegate that can handle various edge cases in an app-specific manner.
*/
@property (nonatomic, weak) id<PKSyncManagerDelegate> delegate;

/**
 The Firebase Auth uid that the user has authenticated with
 */
@property (nonatomic, copy) NSString* userId;

/**
 Returns a random string suitable for using as a sync identifer.
 @return A random string suitable for using as a sync identifer.
 */
+ (NSString *)syncID;

/** @name Creating and Configuring a Sync Manager */

/**
 The designated initializer used to specify the Core Data managed object context and the Dropbox data store that should be synchronized.
 
 @param managedObjectContext The Core Data managed object context the sync manager should listen for changes from.
 @param database The Firebase database reference the sync manager should listen for changes from and write changes to.
 @return A newly initialized `PKSyncManager` object.
 */
- (instancetype)initWithManagedObjectContext:(NSManagedObjectContext *)managedObjectContext databaseRoot:(FIRDatabaseReference *)databaseRoot userId:(NSString*)userId;

/**
 Map multiple Core Data entity names to their corresponding Dropbox data store table name. Replaces all other existing relationships that may have been previously set.
 @param keyedTables Dictionary of key/value pairs where the key is the Core Data entity name and the value is the corresponding Couchbase Lite database table name.
 */
- (void)setTablesForEntityNamesWithDictionary:(NSDictionary *)keyedTables;

/**
 Maps a single Core Data entity name to the corresponding Dropbox data store table name.
 
 Replaces any existing relationship for the given entity name that may have been previously set.
 Will raise an NSInternalInconsistencyException if the entity does not contain a valid sync attribute.
 @param tableID The Couchbase Lite database tableID that the entity name should be mapped to.
 @param entityName The Core Data entity name that should map to the given tableID.
 */
- (void)setTable:(NSString *)tableID forEntityName:(NSString *)entityName;

/**
 Removes the Core Data <-> Couchbase Lite mapping for the given entity name.
 @param entityName The Core Data entity name that should no longer be mapped to Couchbase Lite.
 */
- (void)removeTableForEntityName:(NSString *)entityName;

/** @name Accessing Entity Names and Tables */

/** 
 Returns a dictionary of tables mapped to their corresponding entity names.
 @return A dictionary of tables mapped to their corresponding entity names.
 */
- (NSDictionary *)tablesByEntityName;

/**
 Returns an array of currently mapped Couchbase Lite database tableIDs.
 @return An array of currently mapped Couchbase Lite database tableIDs.
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
 Starts observing changes to the Core Data managed object context and the Couchbase Lite database.
 */
- (void)startObserving;

/**
 Stops observing changes from the Core Data managed object context and the Couchbase Lite database.
 */
- (void)stopObserving;

/**
 Convert a timestamp into a string
 */
- (NSString *)TTTISO8601TimestampFromDate:(NSDate *)date;

/**
 Convert a string into a timestamp
 */
- (NSDate *)TTTDateFromISO8601Timestamp:(NSString *)timestamp;

@end
