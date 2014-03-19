//
//  SnapzChatLib.m
//  SnapzChatLib
//
//  Created by Suwit Arnmanee on 11/03/2014.
//  Copyright (c) 2014 Suwit Arnmanee. All rights reserved.
//
//  History Of Code Changes:
//  Version1.0.0
//  =========
//     11/03/2014 - Initialize
//     15/03/2014 - Added send sendChatMessage(), sendChatMessageComposing and sendChatMessageDelivered.
//                        - Changed if..else in onReceiveNotification() to switch case macros (see. AppClient.hpp)
//
#import "SnapzChatLib.h"
#import "AppClient.hpp"

@interface SnapzChatLib()
{
//#if !__has_feature(objc_arc)
//@private
//    id<SnapzChatEventDelegate> delegate;
//#endif
    
    AppClient *client;
    BOOL loggedIn;
    
}

- (void) onReceiveNotification:(NSNotification *) notification;
@end

// instance of SnapzChatLib
static SnapzChatLib *chatClient;

@implementation SnapzChatLib
@synthesize delegate;

-(id)init {
    self = [super init];
    if (!self) return nil;
    
    // register callback to notification queue
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onReceiveNotification:)
                                                 name:XMPPClientNotification
                                               object:nil];
#if _DEBUG
    NSLog(@"****** <<< SnapzChatLib:: created notification queue >>> ****** %@",
          XMPPClientNotification);
#endif
    [self instance];
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#if _DEBUG
    NSLog(@"****** <<< SnapzChatLib:: destroyed notification queue >>> ****** %@",
          XMPPClientNotification);
#endif
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}


-(void)instance {
    client = AppClient::GetInstance();
}

+(SnapzChatLib*) getInstance {
    if(chatClient == nil)
        chatClient = [[SnapzChatLib alloc] init];
    return chatClient;
}

// sample:
//      server       = jabber.snapz.mobi
//      user          = <snapz_Id>_<session_id>_<token_key>@<server>
//      password = <snapz_id>
//
+(BOOL) connect:(NSString*)tokenKey
  withSessionID:(NSString*)sessionID
           user:(NSString*)user
   withPassword:(NSString*)password
 withServerName:(NSString*)server {
    
    NSString *fullName = [NSString stringWithFormat:@"%@_%@_%@", user, sessionID, tokenKey];
    
 //   AppClient *cli = AppClient::GetInstance();
 //   if(NULL != cli) {
 //       AppClient::Disconnect();
 //   }
    if(chatClient == nil)
        chatClient = [[SnapzChatLib alloc] init];
    
    return (BOOL)(AppClient::Connect([fullName cStringUsingEncoding:NSUTF8StringEncoding],
                                     [password cStringUsingEncoding:NSUTF8StringEncoding],
                                     [server cStringUsingEncoding:NSUTF8StringEncoding]));
  //  NSLog(@"Connect");
  //  return YES;
}

+(BOOL) connect:(NSString*)user
   withPassword:(NSString*)password
 withServerName:(NSString*)server {
    
    
    if(chatClient == nil)
        chatClient = [[SnapzChatLib alloc] init];
    
  //  AppClient *cli = AppClient::GetInstance();
  //  if(NULL != cli) {
  //      cli->Disconnect();
  //  }
    return (BOOL)(AppClient::Connect([user cStringUsingEncoding:NSUTF8StringEncoding],
                                     [password cStringUsingEncoding:NSUTF8StringEncoding],
                                     [server cStringUsingEncoding:NSUTF8StringEncoding]));
}


+(void) disconnect {
    AppClient::Disconnect();
}

+(void) suspend {
    AppClient::Suspend();
}

+(void) resume {
    AppClient::Suspend();
}

-(BOOL) sendChatMessage:(NSString*)user
            withMessage:(NSString*)message
            withSubject:(NSString*)subject
           withResource:(NSString*)resource {
     AppClient *objClient = AppClient::GetInstance();
     return (BOOL)objClient->sendChatMessage([user cStringUsingEncoding:NSUTF8StringEncoding],
                                     [message cStringUsingEncoding:NSUTF8StringEncoding],
                                     [subject cStringUsingEncoding:NSUTF8StringEncoding],
                                     [resource cStringUsingEncoding:NSUTF8StringEncoding]
                                     );
    
}

-(BOOL) sendChatMessageComposing:(NSString*)user
                                 withResource:(NSString*)resource {
    AppClient *objClient = AppClient::GetInstance();
    return (BOOL)objClient->sendChatMessageComposing([user cStringUsingEncoding:NSUTF8StringEncoding],
                                                     [resource cStringUsingEncoding:NSUTF8StringEncoding]);
    
}

-(BOOL) sendChatMessageDelivered:(NSString*)user
            withResource:(NSString*)resource {
    AppClient *objClient = AppClient::GetInstance();
    return (BOOL)objClient->sendChatMessageDelivered([user cStringUsingEncoding:NSUTF8StringEncoding],
                                                     [resource cStringUsingEncoding:NSUTF8StringEncoding]);
}


-(NSString*) version {
    return API_VERSION;
}

- (void) onReceiveNotification:(NSNotification *) notification
{
    NSLog(@"****** SnapzChatLib::=======>onReceiveNotification: %@", [notification name]);
    if (![[notification name] isEqualToString:XMPPClientNotification])
        return;
    
  //  if(self.delegate == nil) {
  //      NSLog(@"!!!!!! SnapzChatLib::self.delegate == nil !!!!!");
  //      return;
  //  }
    
    NSLog(@"onReceiveNotification: %@", notification.userInfo);
    NSDictionary* dict = notification.userInfo;
    for(NSString* key in dict) {
        NSString* event = (NSString*)[dict objectForKey:key];
        // check event key
        if(![key isEqualToString:XMPPClientEventKey]) return;
        
        SWITCH(event) {
            CASE(XMPPEventConnected) {
                // [self.delegate onConnected];
                [[self delegate] onConnected];
                break;
            }
            CASE(XMPPEventDisconnected) {
                NSInteger errorCode = (NSInteger)[dict objectForKey:@"ErrorCode"];
                NSString *errorMsg  = (NSString*)[dict objectForKey:@"ErrorMessage"];
                //[self.delegate onDisconnected:errorCode withErrorMessage:errorMsg ];
                [[self delegate] onDisconnected:errorCode withErrorMessage:errorMsg];
                break;
            }
            CASE(XMPPEventChatMessageReceived) {
                NSString *user       = (NSString*)[dict objectForKey:@"User"];
                NSString *resource   = (NSString*)[dict objectForKey:@"Resource"];
                NSString *subject    = (NSString*)[dict objectForKey:@"Subject"];
                NSString *message    = (NSString*)[dict objectForKey:@"Message"];
                NSString *timestamp  = (NSString*)[dict objectForKey:@"Timestamp"];
                [self.delegate onChatMessage:user
                                withResource:resource
                                 withMessage:message
                                 withSubject:subject
                               withTimestamp:timestamp];
                break;
            }
            CASE(XMPPEventChatMessageComposing) {
                NSString *user       = (NSString*)[dict objectForKey:@"User"];
                NSString *resource   = (NSString*)[dict objectForKey:@"Resource"];
                [self.delegate onChatMessageComposing:user
                                         withResource:resource];

                break;
            }
            CASE(XMPPEventChatMessageDelivered) {
                NSString *user       = (NSString*)[dict objectForKey:@"User"];
                NSString *resource   = (NSString*)[dict objectForKey:@"Resource"];
                [self.delegate onChatMessageDelivered:user
                                         withResource:resource];
                break;
            }
            
            DEFAULT {
                NSLog(@"Unexpected event!");
                break;
            }
        }
                     /*
        {
            if ([value isEqualToString:XMPPEventConnect]) {
                // handle on connected
                
            } else if([value isEqualToString:XMPPEventDisconnect]) {
                // handle on disconnected
                
            } else if([value isEqualToString:XMPPEventChatMessageReceived]) {
                // handle  message received
               
            } else if([value isEqualToString:XMPPEventChatMessageComposing]) {
                // handle message composing
                
            } else if([value isEqualToString:XMPPEventChatMessageDelivered]) {
                // handle message delivered
            }
        }*/
    }
}


@end


