//
//  NSManagedObject+ParcelKit.m
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

#import "FIRFirebaseToManagedObject.h"
#import <FirebaseDatabase/FirebaseDatabase.h>
#import "PKSyncManager.h"
#import "NSNull+PKNull.h"

NSString * const PKInvalidAttributeValueException = @"Invalid attribute value";
static NSString * const PKInvalidAttributeValueExceptionFormat = @"“%@.%@” expected “%@” to be of type “%@” but is “%@”";

@interface PKSyncManager ()
- (NSDate*)valueToDate:(id)value;
@end

@implementation FIRFirebaseToManagedObject

+ (void)setManagedObjectPropertiesOn:(NSManagedObject*)managedObject withRecord:(FIRDataSnapshot *)record syncAttributeName:(NSString *)syncAttributeName manager:(PKSyncManager*)manager
{
    NSString *entityName = [[managedObject entity] name];
    
    NSDictionary *propertiesByName = [[managedObject entity] propertiesByName];    
    NSArray *syncedPropertyNames = nil;
    if (manager.delegate && [manager.delegate respondsToSelector:@selector(syncedPropertyNamesForManagedObject:)]) {
        syncedPropertyNames = [manager.delegate syncedPropertyNamesForManagedObject:managedObject];
    } else {
        syncedPropertyNames = [propertiesByName allKeys];
    }
    
    __weak typeof(managedObject) weakmanagedObject = managedObject;
    
    NSDictionary* recordValues = (NSDictionary*)record.value;
    
    if ((manager.delegate) && ([manager.delegate respondsToSelector:@selector(syncManager:transformRemoteData:forEntityName:reference:)])) {
        // Get the delegate to transform this data
        recordValues = [manager.delegate syncManager:manager transformRemoteData:recordValues forEntityName:entityName reference:record.ref];
    }
    
    [propertiesByName enumerateKeysAndObjectsUsingBlock:^(NSString *propertyName, NSPropertyDescription *propertyDescription, BOOL *stop) {
        typeof(managedObject) strongmanagedObject = weakmanagedObject; if (!strongmanagedObject) return;
        
        if ([propertyName isEqualToString:syncAttributeName] || ![syncedPropertyNames containsObject:propertyName] || [propertyDescription isTransient]) return;
        
        if ([propertyDescription isKindOfClass:[NSAttributeDescription class]]) {
            NSAttributeType attributeType = [(NSAttributeDescription *)propertyDescription attributeType];
            
            id value = [recordValues objectForKey:propertyName];
            
            if ([NSNull isValuePKNull:value]) {
                value = [NSNull null];
            }
            
            if ((value) && (value != [NSNull null])) {
                if ((attributeType == NSStringAttributeType) && (![value isKindOfClass:[NSString class]])) {
                    if ([value respondsToSelector:@selector(stringValue)]) {
                        value = [value stringValue];
                    } else {
                        if (manager.delegate && [manager.delegate respondsToSelector:@selector(managedObject:invalidAttribute:value:expected:)]) {
                            [manager.delegate managedObject:managedObject invalidAttribute:propertyName value:value expected:[NSString class]];
                            return;
                        } else {
                            [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSString class], [value class]];
                        }
                    }
                } else if (((attributeType == NSInteger16AttributeType) ||
                            (attributeType == NSInteger32AttributeType) ||
                            (attributeType == NSInteger64AttributeType)) && (![value isKindOfClass:[NSNumber class]])) {
                    if ([value respondsToSelector:@selector(integerValue)]) {
                        value = [NSNumber numberWithInteger:[value integerValue]];
                    } else {
                        if (manager.delegate && [manager.delegate respondsToSelector:@selector(managedObject:invalidAttribute:value:expected:)]) {
                            [manager.delegate managedObject:managedObject invalidAttribute:propertyName value:value expected:[NSNumber class]];
                            return;
                        } else {
                            [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSNumber class], [value class]];
                        }
                    }
                } else if ((attributeType == NSBooleanAttributeType) && (![value isKindOfClass:[NSNumber class]])) {
                    if ([value respondsToSelector:@selector(boolValue)]) {
                        value = [NSNumber numberWithBool:[value boolValue]];
                    } else {
                        if (manager.delegate && [manager.delegate respondsToSelector:@selector(managedObject:invalidAttribute:value:expected:)]) {
                            [manager.delegate managedObject:managedObject invalidAttribute:propertyName value:value expected:[NSNumber class]];
                            return;
                        } else {
                            [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSNumber class], [value class]];
                        }
                    }
                } else if (((attributeType == NSDoubleAttributeType) || (attributeType == NSFloatAttributeType) || attributeType == NSDecimalAttributeType) && (![value isKindOfClass:[NSNumber class]])) {
                    if ([value respondsToSelector:@selector(doubleValue)]) {
                        value = [NSNumber numberWithDouble:[value doubleValue]];
                    } else {
                        if (manager.delegate && [manager.delegate respondsToSelector:@selector(managedObject:invalidAttribute:value:expected:)]) {
                            [manager.delegate managedObject:managedObject invalidAttribute:propertyName value:value expected:[NSNumber class]];
                            return;
                        } else {
                            [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSNumber class], [value class]];
                        }
                    }
                } else if (attributeType == NSDateAttributeType) {
                
                    NSDate* dateValue = [manager valueToDate:value];
                
                    if (dateValue != nil) {
                        value = dateValue;
                    } else {
                        if (manager.delegate && [manager.delegate respondsToSelector:@selector(managedObject:invalidAttribute:value:expected:)]) {
                            [manager.delegate managedObject:managedObject invalidAttribute:propertyName value:value expected:[NSDate class]];
                            return;
                        } else {
                            [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSDate class], [value class]];
                        }
                    }
                } else if ((attributeType == NSBinaryDataAttributeType) && (![value isKindOfClass:[NSData class]])) {
                    
                    if (manager.delegate && [manager.delegate respondsToSelector:@selector(managedObject:invalidAttribute:value:expected:)]) {
                        [manager.delegate managedObject:managedObject invalidAttribute:propertyName value:value expected:[NSData class]];
                        return;
                    } else {
                        [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSData class], [value class]];
                    }
                }
            } else if (![propertyDescription isOptional]) {
                
                id existingValue = [strongmanagedObject valueForKey:propertyName];
                if (!existingValue) {
                    [NSException raise:PKInvalidAttributeValueException format:@"“%@.%@” expected to not be null", entityName, propertyName];
                } else {
                    // Stick with the current value
                    value = existingValue;
                }
            }
            
            // An absent value just means don't change it
            if (value != nil) {
                if (value == [NSNull null]) {
                    value = nil;
                }
                [strongmanagedObject setValue:value forKey:propertyName];
            }
        } else if ([propertyDescription isKindOfClass:[NSRelationshipDescription class]]) {
            NSRelationshipDescription *relationshipDescription = (NSRelationshipDescription *)propertyDescription;
            NSRelationshipDescription *inverse = [relationshipDescription inverseRelationship];
            NSFetchRequest *fetchRequest = [NSFetchRequest fetchRequestWithEntityName:[[relationshipDescription destinationEntity] name]];
            [fetchRequest setFetchLimit:1];
            
            if ([relationshipDescription isToMany]) {
                // If it's a one-to-many relationship, leave all the relationship business
                // to the "one" side of the equation. Otherwise, carry on and deal with it here
                if ([inverse isToMany]) {
                    NSArray *recordList = [recordValues objectForKey:propertyName];
                    if (recordList && ![recordList isKindOfClass:[NSArray class]]) {
                        [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, recordList, [NSArray class], [recordList class]];
                    }
                    
                    NSMutableArray *recordIdentifiers = [[NSMutableArray alloc] init];
                    for (id value in recordList) {
                        if (![value isKindOfClass:[NSString class]]) {
                            if ([value respondsToSelector:@selector(stringValue)]) {
                                [recordIdentifiers addObject:[value stringValue]];
                            } else {
                                [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, value, [NSString class], [value class]];
                            }
                        } else {
                            [recordIdentifiers addObject:value];
                        }
                    }
                    
                    id relatedObjects = ([relationshipDescription isOrdered] ? [strongmanagedObject mutableOrderedSetValueForKey:propertyName] : [strongmanagedObject mutableSetValueForKey:propertyName]);
                    NSMutableSet *unrelatedObjects = [[NSMutableSet alloc] init];
                    for (NSManagedObject *relatedObject in relatedObjects) {
                        if (![recordIdentifiers containsObject:[relatedObject valueForKey:syncAttributeName]]) {
                            if ((manager.delegate != nil) && ([manager.delegate respondsToSelector:@selector(isRecordSyncable:)])) {
                                if (![manager.delegate isRecordSyncable:relatedObject]) {
                                    // Don't remove links to un-synced objects
                                    continue;
                                }
                            }
                            if (![inverse isOptional]) {
                                // We should only be removing non-optional relationships when
                                // the corresponding record has been deleted
                                if (![relatedObject isDeleted]) {
                                    // Let's keep this relationship
                                    continue;
                                }
                            }
                            [unrelatedObjects addObject:relatedObject];
                        }
                    }
                    [relatedObjects minusSet:unrelatedObjects];
                    
                    NSUInteger recordIndex = 0;
                    for (NSString *identifier in recordIdentifiers) {
                        [fetchRequest setPredicate:[NSPredicate predicateWithFormat:@"%K == %@", syncAttributeName, identifier]];
                        [fetchRequest setIncludesPropertyValues:NO];
                        NSError *error = nil;
                        NSArray *managedObjects = [strongmanagedObject.managedObjectContext executeFetchRequest:fetchRequest error:&error];
                        if (managedObjects) {
                            if ([managedObjects count] == 1) {
                                NSManagedObject *relatedObject = managedObjects[0];
                                
                                if ([relationshipDescription isOrdered]) {
                                    NSUInteger relatedObjectIndex = [relatedObjects indexOfObject:relatedObject];
                                    if (relatedObjectIndex != recordIndex) {
                                        if (relatedObjectIndex != NSNotFound) {
                                            [relatedObjects removeObject:relatedObject];
                                        }
                                        [relatedObjects insertObject:relatedObject atIndex:recordIndex];
                                    }
                                } else {
                                    if (![relatedObjects containsObject:relatedObject]) {
                                        [relatedObjects addObject:relatedObject];
                                    }
                                }
                            }
                        } else {
                            NSLog(@"Error executing fetch request: %@", error);
                            [manager setLastError:error summary:@"Error fetching related objects"];
                        }
                        
                        recordIndex++;
                    };
                }
            } else {
                id identifier = [recordValues objectForKey:propertyName];
                if (identifier) {
                    if (![identifier isKindOfClass:[NSString class]]) {
                        if ([identifier respondsToSelector:@selector(stringValue)]) {
                            identifier = [identifier stringValue];
                        } else {
                            [NSException raise:PKInvalidAttributeValueException format:PKInvalidAttributeValueExceptionFormat, entityName, propertyName, identifier, [NSString class], [identifier class]];
                        }
                    }
                    [fetchRequest setPredicate:[NSPredicate predicateWithFormat:@"%K == %@", syncAttributeName, identifier]];
                    NSError *error = nil;
                    NSArray *managedObjects = [strongmanagedObject.managedObjectContext executeFetchRequest:fetchRequest error:&error];
                    if (managedObjects) {
                        if ([managedObjects count] == 1) {
                            NSManagedObject *relatedObject = managedObjects[0];
                            if (![[strongmanagedObject valueForKey:propertyName] isEqual:relatedObject]) {
                                [strongmanagedObject setValue:relatedObject forKey:propertyName];
                            }
                        }
                    } else {
                        NSLog(@"Error executing fetch request: %@", error);
                        [manager setLastError:error summary:@"Error fetching related object"];
                    }
                } else {
                    [strongmanagedObject setValue:nil forKey:propertyName];
                }
            }
        }
    }];
}
@end
