//
//  SnapzChatLib.h
//  SnapzChatLib
//
//  Created by Suwit Arnmanee on 11/03/2014.
//  Copyright (c) 2014 Suwit Arnmanee. All rights reserved.
//

#import <Foundation/Foundation.h>

//@protocol SnapzChatEventDelegate;

/// protocol
@protocol SnapzChatEventDelegate<NSObject>
@required

/// called when client gets connected
-(void) onConnected;

/// called when client gets disconnected
-(void) onDisconnected:(NSInteger) errorCode withErrorMessage:(NSString*)errorMessage;

/// called when a new message is received
-(void) onChatMessage:(NSString*) user
         withResource:(NSString*) resource
          withMessage:(NSString*) message
          withSubject:(NSString*) subject
        withTimestamp:(NSString*) timestamp;

/// called when 'COMPOSING' event is received
-(void) onChatMessageComposing:(NSString*)user withResource:(NSString*) resource;

/// called when 'DELIVERED' event is received
-(void) onChatMessageDelivered:(NSString*)user withResource:(NSString*) resource;
@end

@interface SnapzChatLib : NSObject {
    id <SnapzChatEventDelegate> delegate;
}

@property(nonatomic, strong) id<SnapzChatEventDelegate> delegate;


/// return API version in format: x.x.x
-(NSString*) version;

/// Instance;
+(SnapzChatLib*) getInstance;

/// connect to chat server (production server)
+(BOOL) connect:(NSString*)tokenKey
  withSessionID:(NSString*)sessionID
           user:(NSString*)user
   withPassword:(NSString*)password
 withServerName:(NSString*)server;

/// connect to chat server (for testing)
+(BOOL) connect:(NSString*)user
   withPassword:(NSString*)password
 withServerName:(NSString*)server;

/// disconnect client from chat server
+(void) disconnect;

/// suspend client connection
+(void) suspend;

/// resume client connection
+(void) resume;

/// sends the specifiled message to user.
-(BOOL) sendChatMessage:(NSString*)user
            withMessage:(NSString*)message
            withSubject:(NSString*)subject
           withResource:(NSString*)resource;

/// sends composing event to given user.
-(BOOL) sendChatMessageComposing:(NSString*)user
                    withResource:(NSString*)resource;

/// sends delivered event to user.
-(BOOL) sendChatMessageDelivered:(NSString*)user
                    withResource:(NSString*)resource;

@end




