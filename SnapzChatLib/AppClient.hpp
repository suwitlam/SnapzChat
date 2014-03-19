//// -*- mode: C++; coding: utf-8; -*-

#ifndef APP_CLIENT_INCLUDED
#define APP_CLIENT_INCLUDED

#include "XMPPClient.hpp"

#define API_VERSION                     @"1.0.0"
#define XMPPClientNotification          @"XMPPClientNotification"
#define XMPPClientEventKey              @"AppClientEvent"
#define XMPPEventConnected              @"onConnected"
#define XMPPEventDisconnected           @"onDisconnected"
#define XMPPEventChatMessageReceived    @"onChatMessageReceived"
#define XMPPEventChatMessageComposing   @"onChatMessageComposing"
#define XMPPEventChatMessageDelivered   @"onChatMessageDelivered"

// switch case macros
#define CASE(str)                       if ([__s__ isEqualToString:(str)])
#define SWITCH(s)                       for (NSString *__s__ = (s); ; )
#define DEFAULT

class AppClient : public XMPPClient {
public:
    static const string DEFAULT_JID;          // "user@jabber.znaap.me"
    static const string DEFAULT_SERVER_NAME;  // "jabber.znaap.me"
    static const int    DEFAULT_SERVER_PORT;  // 5222

    // NOTE: ALL METHODS MUST BE CALLED FROM THE SAME OWNER THREAD THAT CREATES/CONNECTS THIS CLIENT
public:
    static bool Connect(const string& jid, const string& passwd,
                        const string& server = DEFAULT_SERVER_NAME);
    static bool Disconnect();

    static AppClient* GetInstance() {
        return app_client;
    }

    static Config& GetConfig() {
        return config;
    }

    // NOTE: MUST BE ONLY USED in callbacks responsible for taking application to/from background/foreground
    static bool Suspend();
    static bool Resume();

private:
    /// connection callbacks
    virtual void onConnect();
    virtual bool onTlsConnect(const CertInfo& info);
    virtual void onDisconnect(ConnectionError error);

    /// simple chat callbacks
    virtual void onChatMessage(const string& user, const string& resource,
                               const string& message, const string& subject, const char *timestamp);
    virtual void onChatMessageComposing(const string& user, const string& resource);
    virtual void onChatMessageDelivered(const string& user, const string& resource);

    /// group chat callbacks
    virtual bool onGroupChatCreation(const string& group);
    virtual void onGroupChatCreate(const string& group, bool success);
    virtual void onGroupChatDestroy(const string& group, bool success);
    virtual void onGroupChatSubject(const string& group, const string& user, const string& subject);
    virtual void onGroupChatMessage(const string& group, const string& user,
                                    const string& message, const char *timestamp);
    virtual bool onGroupChatInvite(const string& group, const string& user,
                                   const string& reason, const string& passwd);
    virtual void onGroupChatKickResult(const string& group, bool success);
#ifdef XMPP_CLIENT_BAN_ENABLE
    virtual void onGroupChatBanResult(const string& group, bool success);
    virtual void onGroupChatUnbanResult(const string& group, bool success);
#endif // XMPP_CLIENT_BAN_ENABLE
    virtual void onGroupChatUserPresence(const string& group, const string& user,
                                         bool online, const string& reason, int flags);
    virtual void onGroupChatError(const string& group, StanzaError error);
    virtual void onGroupChatInviteDecline(const string& group, const string& user, const string& reason);
    virtual void onGroupChatUsersList(const string& group, const StringList& users);

private:
    explicit AppClient(const Config& config);

    virtual ~AppClient();

#ifdef _DEBUG
private:
    virtual void onClientStart();
    virtual void onClientStop();
#endif // _DEBUG

private:
    static AppClient *app_client;
    static Config config;
    static bool is_suspended;

private:
    AppClient();
    AppClient(const AppClient&);
    const AppClient& operator=(const AppClient&);
};

#endif // APP_CLIENT_INCLUDED

// precompile header: no
// setting Prefix HEader: SnapzChatLib/SnapzChatLib-Prefix.pch
