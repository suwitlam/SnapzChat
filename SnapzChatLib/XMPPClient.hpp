//// -*- mode: C++; coding: utf-8; -*-

#ifndef XMPP_CLIENT_INCLUDED
#define XMPP_CLIENT_INCLUDED

#include <gloox/gloox.h>
#include <stdexcept>

#include <pthread.h>

using namespace std;
using namespace gloox;

namespace gloox {
    class Client;
} // namespace gloox

class XMPPClient
{
public:
    class Error : public runtime_error
    {
    public:
        explicit Error(const string& error);
        explicit Error(const char *error);
        explicit Error(int errnum);

    private:
        Error();
        const Error& operator=(const Error&);
    };

    struct Config
    {
        explicit Config(const string& jid, const string& password);

        explicit Config();
        explicit Config(const Config& config);
        const Config& operator=(const Config&);

        string jid;
        string passwd;            // empty string

        string server;            // generated from jid
        int port;                 // 5222

        TLSPolicy tls_policy;     // TLSOptional
        StringList ca_certs;      // empty list

        string groupchat_server;  // generated from jid

        int recv_timeout;         // no timeout (-1), otherwise in milliseconds
    };

    explicit XMPPClient(const Config& config);

    virtual ~XMPPClient() = 0;

    bool update(int timeout = -1);

    const Config& getConfig() const {
        return config;
    }

public:
    /// connection methods
    bool connect(bool start_thread = true);
    void disconnect();

    /// simple chat methods
    bool sendChatMessage(const string& user, const string& message, const string& subject = "", const string& resource = "");
    bool sendChatMessageComposing(const string& user, const string& resource = "");
    bool sendChatMessageDelivered(const string& user, const string& resource = "");

    /// group chat methods
    struct GroupChatConfig
    {
        explicit GroupChatConfig();
        explicit GroupChatConfig(const GroupChatConfig& config);
        const GroupChatConfig& operator=(const GroupChatConfig& config);

        static const string CONF_TITLE;                               // empty string
        static const string CONF_DESCRIPTION;                         // empty string
        static const bool   CONF_IS_PERSISTENT;                       // false
        static const string CONF_PASSWD;                              // empty string
        static const string CONF_MAX_NUMBER_USERS;                    // "200"
        static const bool   CONF_IS_MEMBERS_ONLY;                     // false
        static const bool   CONF_IS_USER_SUBJECT_CHANGE_ALLOWED;      // true
        static const bool   CONF_IS_USER_INVITE_SEND_ALLOWED;         // false
        static const bool   CONF_IS_USER_SEND_VOICE_REQUEST_ALLOWED;  // true

        string title;
        string description;
        bool is_persistent;
        string passwd;
        string max_number_users;
        bool is_members_only;
        bool is_user_subject_change_allowed;
        bool is_user_invite_send_allowed;
        bool is_user_send_voice_request_allowed;
    };

#ifdef XMPP_CLIENT_INVITE_DECLINE_ENABLE
    bool declineGroupChatInvitation(const string& group, const string& user, const string& reason = "");
#endif // XMPP_CLIENT_INVITE_DECLINE_ENABLE

    bool beginGroupChat(const string& group, const string& passwd = "",
                        int history_messages = -1, const string& history_since = "");
    bool endGroupChat(const string& group, const string& reason = "");
    bool destroyGroupChat(const string& group, const string& reason = "");
    bool configureGroupChat(const string& group, const GroupChatConfig *config = 0);
    bool cancelGroupChatCreation(const string& group);
    bool setGroupChatSubject(const string& group, const string& subject);
    bool sendGroupChatMessage(const string& group, const string& message);
    bool inviteToGroupChat(const string& group, const string& user, const string& reason = "");
    bool kickFromGroupChat(const string& group, const string& user, const string& reason = "");

#ifdef XMPP_CLIENT_BAN_ENABLE
    bool banFromGroupChat(const string& group, const string& user, const string& reason = "");
    bool unbanFromGroupChat(const string& group, const string& user);
#endif // XMPP_CLIENT_BAN_ENABLE

    bool listGroupChatUsers(const string& group);

protected:
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

protected:
    gloox::Client* getXmpp() const;

protected:
    virtual bool handleUpdateError(ConnectionState state, ConnectionError error);

    virtual void onClientStart();
    virtual void onClientStop();

private:
    bool internalUpdate(int timeout);

private:
    class ClientImpl;

    class ChatImpl;
    struct ChatSession;

    class GroupChatImpl;
    struct GroupChatSession;

    Config config;
    ClientImpl *impl;

    volatile bool is_connected;

    static void* event_loop(void *data);
    pthread_t event_loop_thread;
    volatile bool is_running;

    int recv_timeout;

private:
    XMPPClient();
    XMPPClient(const XMPPClient&);
    const XMPPClient& operator=(const XMPPClient&);
};

#endif // XMPP_CLIENT_INCLUDED
