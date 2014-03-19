//// -*- mode: C++; coding: utf-8; -*-

#include "AppClient.hpp"

#include <gloox/client.h>

#include <vector>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#define ROOT_CA_CERTIFICATE_NAME "sunca.cer"

const string AppClient::DEFAULT_JID          = "user1@jabber.snapz.mobi";
const string AppClient::DEFAULT_SERVER_NAME  = "jabber.snapz.mobi";
const int    AppClient::DEFAULT_SERVER_PORT  = 5222;

//// !!!WARNING!!! NEVER BLOCK IN ANY OF THE HANDLERS BELOW

#ifdef _DEBUG
void AppClient::onClientStart()
{
    ::fprintf(stderr, "##### AppClient::onClientStart()\n");
}

void AppClient::onClientStop()
{
    ::fprintf(stderr, "##### AppClient::onClientStop()\n");
}
#endif // _DEBUG

//// called when XMPP client gets connected
void AppClient::onConnect()
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### Enter AppClient::onConnect()\n");
#endif // _DEBUG

    //// add code for performing some actions on connect
    NSDictionary* itemDetails = [[NSDictionary alloc]
                                 initWithObjectsAndKeys:
                                 [NSString stringWithFormat:@"%@", XMPPEventConnected], XMPPClientEventKey,
                                 nil];
    
    [[NSNotificationCenter defaultCenter]
     postNotificationName: XMPPClientNotification
     object: nil userInfo:itemDetails];
    
#ifdef _DEBUG
    ::fprintf(stderr, "##### Leave AppClient::onConnect()\n");
#endif // _DEBUG
    
}

//// called when XMPP client connects with encryption
bool AppClient::onTlsConnect(const CertInfo& info)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onTlsConnect(): cipher='%s'\n",
              info.cipher.c_str());
#endif // _DEBUG
    ////
    //// add code for handling/checking SSL certificate:
    //// return 'true' if accepted and 'false' otherwise
    ////
    // accept/trust any certificate
    return true;
}

/// called when XMPP client gets disconnected
void AppClient::onDisconnect(ConnectionError error)
{
    
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onDisconnect(): ");
    switch(error) {
        case ConnUserDisconnected:
            ::fprintf(stderr, "ConnUserDisconnected\n");
            break;
            
        case ConnStreamError:
            ::fprintf(stderr, "ConnStreamError: cause=%d\n",
                      (int)getXmpp()->streamError());
            break;
        case ConnAuthenticationFailed:
            ::fprintf(stderr, "ConnAuthenticationFailed: cause=%d\n",
                      (int)getXmpp()->authError());
            break;
        case ConnDnsError:
            ::fprintf(stderr, "ConnDnsError\n");
            break;
            
        default:
            ::fprintf(stderr, "error=%d\n", (int)error);
            break;
    }
#endif // _DEBUG
    
    NSString *errmsg = @"";
    switch(error) {
        case ConnUserDisconnected:
            errmsg = @"ConnUserDisconnected";
            break;
            
        case ConnStreamError:
            errmsg = [NSString stringWithFormat:@"ConnStreamError: cause=%d", (int)getXmpp()->streamError()];
            break;
            
        case ConnAuthenticationFailed:
            errmsg = [NSString stringWithFormat:@"ConnAuthenticationFailed: cause=%d",
                      (int)getXmpp()->authError()];
            break;
            
        case ConnDnsError:
            errmsg = [NSString stringWithFormat:@"ConnDnsError"];
            break;
            
        default:
            errmsg = [NSString stringWithFormat:@"error=%d\n", (int)error];
            break;
    }
    
    //// add code for performing some actions on disconnect
    /* error = 0 ( ConNoError)**< Not really an error. Everything went just fine. */
    NSDictionary* itemDetails = [[NSDictionary alloc]
                                 initWithObjectsAndKeys:
                                 [NSString stringWithFormat:@"%@", XMPPEventDisconnected], XMPPClientEventKey,
                                 [NSString stringWithFormat:@"%d", (int)error], @"ErrorCode",
                                 [NSString stringWithFormat:@"%@", errmsg],     @"ErrorMessage",
                                 nil];
    
    [[NSNotificationCenter defaultCenter]
    postNotificationName: XMPPClientNotification object: nil userInfo:itemDetails];
    
}

//// called when a new message is received
void AppClient::onChatMessage(const string& user, const string& resource,
                              const string& message, const string& subject, const char *timestamp)
{

//#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onChatMessage(): "
            "user='%s' resource='%s' timestamp='%s' subject='%s' body='%s'\n",
            user.c_str(), resource.c_str(),
            (timestamp ? timestamp : "n/a"),
            subject.c_str(), message.c_str());
    
    // send 'DELIVERED' notification
    if(XMPPClient::sendChatMessageDelivered(user)) {
        ::fprintf(stderr, "##### AppClient::onChatMessage(): "
                  "send 'DELIVERED' notification to user='%s'\n", user.c_str());
    }
    else {
        ::fprintf(stderr, "##### AppClient::onChatMessage(): "
                "FAILED to send 'DELIVERED' notification to user='%s'\n", user.c_str());
    }

//#endif // _DEBUG
    NSString *fromUser        = [NSString stringWithUTF8String:user.c_str()];
    NSString *strResource     = [NSString stringWithUTF8String:resource.c_str()];
    NSString *msg             = [NSString stringWithUTF8String:message.c_str()];
    NSString *strSubject      = [NSString stringWithUTF8String:subject.c_str()];
    NSString *strTimestamp    = [NSString stringWithUTF8String:(timestamp ? timestamp : "n/a")];
    NSDictionary* itemDetails = [[NSDictionary alloc]
                                 initWithObjectsAndKeys:
                                 [NSString stringWithFormat:@"%@", XMPPEventChatMessageReceived], XMPPClientEventKey,
                                 [NSString stringWithFormat:@"%@", fromUser],     @"User",
                                 [NSString stringWithFormat:@"%@", strResource],  @"Resource",
                                 [NSString stringWithFormat:@"%@", strSubject],   @"Subject",
                                 [NSString stringWithFormat:@"%@", msg],          @"Message",
                                 [NSString stringWithFormat:@"%@", strTimestamp], @"Timestamp",
                                 nil];
    
    [[NSNotificationCenter defaultCenter]
     postNotificationName: XMPPClientNotification
     object: nil userInfo:itemDetails];
}

//// called when 'COMPOSING' event is received
void AppClient::onChatMessageComposing(const string& user, const string& resource)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onChatMessageComposing(): "
              "user='%s' resource='%s'\n", user.c_str(), resource.c_str());
#endif // _DEBUG
    
    NSString *msg  = [NSString stringWithUTF8String:resource.c_str()];
    NSString *fromUser  = [NSString stringWithUTF8String:user.c_str()];
    // NSString *receivedTime      = [NSString stringWithUTF8String:timestamp];
    NSDictionary* itemDetails = [[NSDictionary alloc]
                                 initWithObjectsAndKeys:
                                 [NSString stringWithFormat:@"%@", XMPPEventChatMessageComposing], XMPPClientEventKey,
                                 [NSString stringWithFormat:@"%@", fromUser], @"User",
                                 [NSString stringWithFormat:@"%@", msg],      @"Resource",
                                 nil];
    
    [[NSNotificationCenter defaultCenter]
     postNotificationName: XMPPClientNotification
     object: nil userInfo:itemDetails];
}

//// called when 'DELIVERED' event is received
void AppClient::onChatMessageDelivered(const string& user, const string& resource)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onChatMessageDelivered(): "
              "user='%s' resource='%s'\n", user.c_str(), resource.c_str());
#endif // _DEBUG
    
    NSString *msg  = [NSString stringWithUTF8String:resource.c_str()];
    NSString *fromUser  = [NSString stringWithUTF8String:user.c_str()];
    // NSString *receivedTime      = [NSString stringWithUTF8String:timestamp];
    NSDictionary* itemDetails = [[NSDictionary alloc]
                                 initWithObjectsAndKeys:
                                 [NSString stringWithFormat:@"%@", XMPPEventChatMessageDelivered], XMPPClientEventKey,
                                 [NSString stringWithFormat:@"%@", fromUser], @"User",
                                 [NSString stringWithFormat:@"%@", msg],      @"Resource",
                                 nil];
    
    [[NSNotificationCenter defaultCenter]
     postNotificationName: XMPPClientNotification
     object: nil userInfo:itemDetails];
}

//// called when a new group is being created
bool AppClient::onGroupChatCreation(const string& group)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatCreation(): "
              "group='%s'\n", group.c_str());

    // if TRUE accepts the default room configuration
    // otherwise the group must be configured later
#endif // _DEBUG
    return false;
}

//// called when a new group is created
void AppClient::onGroupChatCreate(const string& group, bool success)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatCreate(): "
              "group='%s' success='%s'\n",
               group.c_str(), (success ? "yes" : "no"));
#endif // _DEBUG
}

//// called when a group is destroyed
void AppClient::onGroupChatDestroy(const string& group, bool success)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatDestroy(): "
              "group='%s' success='%s'\n",
              group.c_str(), (success ? "yes" : "no"));
#endif // _DEBUG
}

//// called when a new group subject is set
void AppClient::onGroupChatSubject(const string& group, const string& user, const string& subject)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatSubject(): "
              "group='%s' user='%s' subject='%s'\n",
              group.c_str(), user.c_str(), subject.c_str());
#endif // _DEBUG
}

//// called when a new group message is received
void AppClient::onGroupChatMessage(const string& group, const string& user,
                                   const string& message, const char *timestamp)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatMessage(): "
            "group='%s' user='%s' timestamp='%s' message='%s'\n",
            group.c_str(), user.c_str(),
            (timestamp ? timestamp : "n/a"), message.c_str());
#endif // _DEBUG
}

//// called when a new invitation to a new group is received
bool AppClient::onGroupChatInvite(const string& group, const string& user,
                                  const string& reason, const string& passwd)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatInvite(): "
              "group='%s' user='%s' reason='%s' passwd='%s'\n",
              group.c_str(), user.c_str(), reason.c_str(), passwd.c_str());
#endif // _DEBUG

#ifdef XMPP_CLIENT_INVITE_DECLINE_ENABLE
    // if TRUE accept the invitation
    return true;
#else
    // any value
    return true;
#endif // XMPP_CLIENT_INVITE_DECLINE_ENABLE
}

//// called when there is an error while performing group chat
void AppClient::onGroupChatError(const string& group, StanzaError error)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatError(): "
              "group='%s' error=%d\n",
              group.c_str(), (int)error);
#endif // _DEBUG
}

//// called when an invitation is declined
void AppClient::onGroupChatInviteDecline(const string& group, const string& user, const string& reason)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatInviteDecline(): "
              "group='%s' user='%s' reason='%s'\n",
              group.c_str(), user.c_str(), reason.c_str());
#endif // _DEBUG
}

//// called when a user gets kicked from the group
void AppClient::onGroupChatKickResult(const string& group, bool success)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatKickResult(): "
              "group='%s' status='%s'\n",
              group.c_str(), (success ? "ok" : "error"));
#endif // _DEBUG
}

#ifdef XMPP_CLIENT_BAN_ENABLE
//// called when a user gets banned from the group
void AppClient::onGroupChatBanResult(const string& group, bool success)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatBanResult(): "
              "group='%s' status='%s'\n",
              group.c_str(), (success ? "ok" : "error"));
#endif // _DEBUG
}

//// called when a user gets unbanned from the group
void AppClient::onGroupChatUnbanResult(const string& group, bool success)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatUnbanResult(): "
              "group='%s' status='%s'\n",
              group.c_str(), (success ? "ok" : "error"));
#endif // _DEBUG
}
#endif // XMPP_CLIENT_BAN_ENABLE

//// called when a user changes his/her online status
void AppClient::onGroupChatUserPresence(const string& group, const string& user,
                                        bool online, const string& reason, int flags)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatUserPresence(): "
              "group='%s' user='%s' online='%s' reason='%s' case=",
              group.c_str(), user.c_str(),
              (online ? "yes" : "no"), reason.c_str());

    if(flags & UserKicked) {
        ::fprintf(stderr, "user_kicked,");
    }
    if(flags & UserBanned) {
        ::fprintf(stderr, "user_banned,");
    }
    if(flags & UserRoomDestroyed) {
        ::fprintf(stderr, "room_destroyed,");
    }
    if(flags & UserNewRoom) {
        ::fprintf(stderr, "new_room,");
    }

    ::fprintf(stderr, "\n");
#endif
}

//// called when a list of users from the group is retrieved
void AppClient::onGroupChatUsersList(const string& group, const StringList& users)
{
#ifdef _DEBUG
    ::fprintf(stderr, "##### AppClient::onGroupChatUsersList(): "
              "group='%s' users=[", group.c_str());

    StringList::const_iterator it;
    for(it = users.begin(); it != users.end(); ++it) {
        ::fprintf(stderr, "'%s' ", it->c_str());
    }

    ::fprintf(stderr, "]\n");
#endif // _DEBUG
}

#ifdef XMPP_CLIENT_PUBSUB_ENABLE
/// pubsub callbacks
//// called when subscription to the node is complete
void AppClient::onSubscribeToPubsubNode(const string& node, bool success)
{
    ::fprintf(stderr, "##### AppClient::onSubscribeToPubsubNode(): "
              "node='%s' success='%s'\n",
              node.c_str(), (success ? "yes" : "no"));
}

//// called when unsubscription from the node is complete
void AppClient::onUnsubscribeFromPubsubNode(const string& node, bool success)
{
    ::fprintf(stderr, "##### AppClient::onUnsubscribeFromPubsubNode(): "
             "node='%s' success='%s'\n",
             node.c_str(), (success ? "yes" : "no"));
}

//// called when node is created
void AppClient::onCreatePubsubNode(const string& node, bool success)
{
    ::fprintf(stderr, "##### AppClient::onCreatePubsubNode(): "
              "node='%s' success='%s'\n",
              node.c_str(), (success ? "yes" : "no"));
}

//// called when node is destroyed
void AppClient::onDestroyPubsubNode(const string& node, bool success)
{
    ::fprintf(stderr, "##### AppClient::onDestroyPubsubNode(): "
              "node='%s' success='%s'\n",
              node.c_str(), (success ? "yes" : "no"));
}

//// called when items are purged from the node
void AppClient::onPurgePubsubNode(const string& node, bool success)
{
    ::fprintf(stderr, "##### AppClient::onPurgePubsubNode(): "
              "node='%s' success='%s'\n",
              node.c_str(), (success ? "yes" : "no"));
}

//// called when publications to the node is complete
void AppClient::onPublishToPubsubNode(const string& node, bool success)
{
    ::fprintf(stderr, "##### AppClient::onPublishToPubsubNode(): "
              "node='%s' success='%s'\n",
               node.c_str(), (success ? "yes" : "no"));
}
#endif // XMPP_CLIENT_PUBSUB_ENABLE

static vector<string> g_string_split(const string& str_, char delimiter)
{
    const char *str = str_.c_str();
    vector<string> result;
    const char *begin;

    do {
        begin = str;

        while(*str != delimiter && *str) {
            str++;
        }

        result.push_back(string(begin, str));
    } while (0 != *str++);

    return result;
}

static bool g_update_configuration(XMPPClient::Config *config, const string& server)
{
    // setup root CA certificate
    config->ca_certs.push_back(ROOT_CA_CERTIFICATE_NAME);
    config->tls_policy = TLSRequired;
    config->recv_timeout = 1000;  // in milliseconds

    if(!server.empty()) {
        config->server = AppClient::DEFAULT_SERVER_NAME;
        config->port = AppClient::DEFAULT_SERVER_PORT;

        // parse 'hostname[:port]' form
        vector<string> hostname_port = g_string_split(server, ':');
        if(!hostname_port.empty()) {
            config->server = hostname_port[0];
            if(hostname_port.size() > 1) {
                config->port = ::atoi(hostname_port[1].c_str());
            }
        }
    }

    return true;
}

AppClient::AppClient(const Config& config)
: XMPPClient(config)
{
    
}

AppClient::~AppClient()
{
    XMPPClient::disconnect();
    // no resources to deallocate
}

AppClient* AppClient::app_client = 0;

XMPPClient::Config AppClient::config;

bool AppClient::is_suspended = false;

bool AppClient::Connect(const string& jid, const string& passwd, const string& server)
{
    NSLog(@"AppClient::Connect");
    
    if(app_client == 0) {
        try {
            config = Config(jid, passwd);
            is_suspended = false;

            if(g_update_configuration(&config, server)) {
                app_client = new AppClient(config);

                if(!app_client->connect()) {
                    delete app_client;
                    app_client = 0;
                }
            }
        }
        catch(...) {
            app_client = 0; // placeholder
        }
        
    }
    NSLog(@"app_client: %d", (app_client!=0));
    return (app_client != 0);
}

bool AppClient::Disconnect()
{
    if(app_client != 0) {
        delete app_client;
        app_client = 0;

        is_suspended = false;
        return true;
    }

    return false;
}

bool AppClient::Suspend()
{
    if(!is_suspended) {
        if(app_client != 0) {
            delete app_client;
            app_client = 0;

            is_suspended = true;
            return true;
        }
    }

    return false;
}

bool AppClient::Resume()
{
    if(is_suspended) {
        assert(app_client == 0);
        is_suspended = false;

        try {
            app_client = new AppClient(config);

            if(!app_client->connect()) {
                delete app_client;
                app_client = 0;
            }
        }
        catch(...) {
            app_client = 0; // placeholder
        }

        return (app_client != 0);
    }

    return false;
}
