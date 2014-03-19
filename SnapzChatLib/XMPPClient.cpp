//// -*- mode: C++; coding: utf-8; -*-

#include "XMPPClient.hpp"

#include <gloox/error.h>
#include <gloox/client.h>
#include <gloox/disco.h>
#include <gloox/connectionlistener.h>
#include <gloox/message.h>
#include <gloox/messagesession.h>
#include <gloox/messagesessionhandler.h>
#include <gloox/messagehandler.h>
#include <gloox/messageevent.h>
#include <gloox/messageeventhandler.h>
#include <gloox/messageeventfilter.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>
#include <gloox/mucroomconfighandler.h>
#include <gloox/mucinvitationhandler.h>
#include <gloox/dataformitem.h>
#include <gloox/mutex.h>
#include <gloox/mutexguard.h>

#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sched.h>

using namespace gloox::util;

class XMPPClient::ClientImpl :
    public ConnectionListener
#ifdef _DEBUG
   ,public LogHandler
#endif // _DEBUG
{
public:
    explicit ClientImpl(XMPPClient *client, const Config& config);

    virtual ~ClientImpl();

    XMPPClient* getClient() const {
        return client;
    }

    gloox::Client* getXmpp() const {
        return xmpp;
    }

    ChatImpl* getChatImpl() const {
        return chat_impl;
    }

    GroupChatImpl* getGroupChatImpl() const {
        return group_chat_impl;
    }

public:
    // ConnectionListener
    virtual void onConnect();
    virtual void onDisconnect(ConnectionError error);
    virtual bool onTLSConnect(const CertInfo& info);

#ifdef _DEBUG
    // LogHandler
    virtual void handleLog(LogLevel level, LogArea area, const string& message);
#endif // _DEBUG

private:
    XMPPClient *client;
    gloox::Client *xmpp;

    ChatImpl *chat_impl;
    GroupChatImpl *group_chat_impl;

private:
    ClientImpl();
    ClientImpl(const ClientImpl&);
    const ClientImpl& operator=(const ClientImpl&);
};

struct XMPPClient::ChatSession
{
    explicit ChatSession(ClientImpl *impl, MessageSession *message_session);
    explicit ChatSession(ClientImpl *impl, const string& user, const string& resource = "");

    virtual ~ChatSession();

    string session_id;  // JID's username part
    string resource;    // JID's resource part
    MessageSession *session;
    MessageEventFilter *message_event_filter;

private:
    ClientImpl *impl;

private:
    ChatSession();
    ChatSession(const ChatSession&);
    const ChatSession& operator=(const ChatSession&);
};

class XMPPClient::ChatImpl :
    public MessageSessionHandler,
    public MessageHandler,
    public MessageEventHandler
{
public:
    explicit ChatImpl(XMPPClient *client, ClientImpl *impl);

    virtual ~ChatImpl();

public:
    // MessageSessionHandler
    virtual void handleMessageSession(MessageSession *session);

    // MessageHandler
    virtual void handleMessage(const Message& message, MessageSession *session);

    // MessageEventHandler
    virtual void handleMessageEvent(const JID& from, MessageEventType event);

public:
    bool sendChatMessage(const string& user, const string& message, const string& subject, const string& resource);
    bool sendChatMessageComposing(const string& user, const string& resource);
    bool sendChatMessageDelivered(const string& user, const string& resource);

    void handlePrivateChatMessage(const string& user, const string& room, const Message& message);

    void disposeChatSessions();

private:
    void handleChatSession(MessageSession *session);
    void handleChatMessageEvent(const JID& from, MessageEventType event);
    ChatSession* findChatSession(const string& id);

private:
    typedef map<string, ChatSession*> ChatSessions;
    ChatSessions chat_sessions;
    gloox::util::Mutex chat_sessions_lock;

private:
    XMPPClient *client;
    ClientImpl *impl;

private:
    ChatImpl();
    ChatImpl(const ChatImpl&);
    const ChatImpl& operator=(const ChatImpl&);
};

struct XMPPClient::GroupChatSession
{
    explicit GroupChatSession(XMPPClient::ClientImpl *impl, const string& group);

    virtual ~GroupChatSession();

    string session_id;  // group name
    bool is_joined;
    MUCRoom *room;

    enum CreationState {CREATION_STATE_NONE, CREATION_STATE_PENDING, CREATION_STATE_COMPLETE};
    CreationState creation_state;

private:
    ClientImpl *impl;

private:
    GroupChatSession();
    GroupChatSession(const GroupChatSession&);
    const GroupChatSession& operator=(const GroupChatSession&);
};

class XMPPClient::GroupChatImpl :
    public MUCRoomHandler,
    public MUCInvitationHandler,
    public MUCRoomConfigHandler
{
public:
    explicit GroupChatImpl(XMPPClient *client, ClientImpl *impl);

    virtual ~GroupChatImpl();

public:
    // MUCRoomHandler
    virtual void handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence& presence);
    virtual void handleMUCMessage(MUCRoom *room, const Message& msg, bool priv);
    virtual bool handleMUCRoomCreation(MUCRoom *room);
    virtual void handleMUCSubject(MUCRoom *room, const string& nick, const string& subject);
    virtual void handleMUCInviteDecline(MUCRoom *room, const JID& invitee, const string& reason);
    virtual void handleMUCError(MUCRoom *room, StanzaError error);
    virtual void handleMUCInfo(MUCRoom *room, int features, const string& name, const DataForm *infoForm);
    virtual void handleMUCItems(MUCRoom *room, const Disco::ItemList& items);

    // MUCInvitationHandler
    virtual void handleMUCInvitation(const JID& room, const JID& from, const string& reason,
                                     const string& body, const string& password, bool cont, const string& thread);

    // MUCRoomConfigHandler
    virtual void handleMUCConfigList(MUCRoom* room, const MUCListItemList& items, MUCOperation operation);
    virtual void handleMUCConfigForm(MUCRoom* room, const DataForm& form);
    virtual void handleMUCConfigResult(MUCRoom* room, bool success, MUCOperation operation);
    virtual void handleMUCRequest(MUCRoom* room, const DataForm& form);

public:
#ifdef XMPP_CLIENT_INVITE_DECLINE_ENABLE
    bool declineGroupChatInvitation(const string& group, const string& user, const string& reason);
#endif // XMPP_CLIENT_INVITE_DECLINE_ENABLE

    bool configureGroupChat(const string& group, const GroupChatConfig *config);
    bool cancelGroupChatCreation(const string& group);
    bool beginGroupChat(const string& group, const string& passwd,
                       int history_messages, const string& history_since);
    bool endGroupChat(const string& group, const string& reason);
    bool destroyGroupChat(const string& group, const string& reason);
    bool setGroupChatSubject(const string& group, const string& subject);
    bool sendGroupChatMessage(const string& group, const string& message);
    bool inviteToGroupChat(const string& group, const string& user, const string& reason);
    bool kickFromGroupChat(const string& group, const string& user, const string& reason);

#ifdef XMPP_CLIENT_BAN_ENABLE
    bool banFromGroupChat(const string& group, const string& user, const string& reason);
    bool unbanFromGroupChat(const string& group, const string& user);
#endif // XMPP_CLIENT_BAN_ENABLE

    bool listGroupChatUsers(const string& group);

    void disposeGroupChatSessions();

private:
    GroupChatSession* findGroupChatSession(const string& id);

private:
    typedef map<string, GroupChatSession*> GroupChatSessions;
    GroupChatSessions chat_sessions;
    gloox::util::Mutex chat_sessions_lock;

    GroupChatConfig config;

private:
    XMPPClient *client;
    ClientImpl *impl;

private:
    GroupChatImpl();
    GroupChatImpl(const GroupChatImpl&);
    const GroupChatImpl& operator=(const GroupChatImpl&);
};

/// XMPPClient::ClientImpl
XMPPClient::ClientImpl::ClientImpl(XMPPClient *client_, const Config& config)
    : client(client_), xmpp(0),
      chat_impl(0), group_chat_impl(0)
{
    xmpp = new gloox::Client(config.jid, config.passwd, config.port);

    if(!config.server.empty()) {
        xmpp->setServer(config.server);
    }

    xmpp->registerConnectionListener(this);

    int message_events = gloox::MessageEventDelivered | gloox::MessageEventComposing;
    xmpp->registerStanzaExtension(new gloox::MessageEvent(message_events));

    xmpp->registerStanzaExtension(new gloox::DelayedDelivery());

    xmpp->disableRoster();

    // register single chat handler
    try {
        chat_impl = new ChatImpl(client, this);
        xmpp->registerMessageSessionHandler(chat_impl, 0);
    }
    catch(...) {
        delete xmpp;
        throw;
    }

    // register group chat handler
    try {
        group_chat_impl = new GroupChatImpl(client, this);
        xmpp->registerMUCInvitationHandler(group_chat_impl);
    }
    catch(...) {
        delete chat_impl;
        delete xmpp;
        throw;
    }

    // set TLS policy for connection
    xmpp->setTls(config.tls_policy);

    // set CA certs if provided
    if(!config.ca_certs.empty()) {
        xmpp->setCACerts(config.ca_certs);
    }

#ifdef _DEBUG
    xmpp->logInstance().registerLogHandler(LogLevelDebug, LogAreaAll, this);
#endif // _DEBUG
}

XMPPClient::ClientImpl::~ClientImpl()
{
    delete group_chat_impl;
    delete chat_impl;
    delete xmpp;
}

// ConnectionListener
void XMPPClient::ClientImpl::onConnect()
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> ClientImpl::onConnect()\n");
#endif // _DEBUG

    client->onConnect();
}

void XMPPClient::ClientImpl::onDisconnect(ConnectionError error)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> ClientImpl::onDisconnect(): "
              "error=%d\n", (int)error);
#endif // _DEBUG

    client->onDisconnect(error);
}

bool XMPPClient::ClientImpl::onTLSConnect(const CertInfo& info)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> ClientImpl::onTLSConnect()\n");
#endif // _DEBUG

    return client->onTlsConnect(info);
}

#ifdef _DEBUG
static const char* g_log_level_string(LogLevel level)
{
    switch(level) {
    case LogLevelDebug:
        return "DEBUG";
    case LogLevelWarning:
        return "WARN";
    case LogLevelError:
    default:
        break;
    }
    return "ERROR";
}

static const char* g_log_area_string(LogArea area)
{
    switch(area) {
    case LogAreaClassParser:
        return "CLASS_PARSER";
    case LogAreaClassConnectionTCPBase:
        return "CLASS_CONNECTION_TCP_BASE";
    case LogAreaClassClient:
        return "CLASS_CLIENT";
    case LogAreaClassClientbase:
        return "CLASS_CLIENT_BASE";
    case LogAreaClassComponent:
        return "CLASS_COMPONENT";
    case LogAreaClassDns:
        return "CLASS_DNS";
    case LogAreaClassConnectionHTTPProxy:
        return "CLASS_CONNECTION_HTTP_PROXY";
    case LogAreaClassConnectionSOCKS5Proxy:
        return "CLASS_CONNECTION_SOCKS5_PROXY";
    case LogAreaClassConnectionTCPClient:
        return "CLASS_CONNECTION_TCP_CLIENT";
    case LogAreaClassConnectionTCPServer:
        return "CLASS_CONNECTION_TCP_SERVER";
    case LogAreaClassS5BManager:
        return "CLASS_S5B_MANAGER";
    case LogAreaClassSOCKS5Bytestream:
        return "CLASS_SOCKS5_BYTE_STREAM";
    case LogAreaClassConnectionBOSH:
        return "CLASS_CONNECTION_BOSH";
    case LogAreaClassConnectionTLS:
        return "CLASS_CONNECTION_TLS";
    case LogAreaAllClasses:
        return "ALL_CLASSES";
    case LogAreaXmlIncoming:
        return "XML_INCOMING";
    case LogAreaXmlOutgoing:
        return "XML_OUTGOING";
    case LogAreaUser:
        return "USER";
    default:
        break;
    }
    return "UNKNOWN";
}

void XMPPClient::ClientImpl::handleLog(LogLevel level, LogArea area, const string& message)
{
    ::fprintf(stderr, ">>> %s >>> %s >>> %s\n",
              g_log_level_string(level), g_log_area_string(area), message.c_str());
}
#endif // _DEBUG

/// XMPPClient::ChatSession
XMPPClient::ChatSession::ChatSession(ClientImpl *impl_, MessageSession *session_)
    : session_id(session_->target().username()),
      resource(session_->target().resource()),
      session(session_),
      message_event_filter(0),
      impl(impl_)
{
    assert(!resource.empty());

    session->registerMessageHandler(impl->getChatImpl());

    try {
        message_event_filter = new MessageEventFilter(session);
        message_event_filter->registerMessageEventHandler(impl->getChatImpl());
    }
    catch(...) {
        impl->getXmpp()->disposeMessageSession(session);

        throw;
    }
}

XMPPClient::ChatSession::ChatSession(ClientImpl *impl_, const string& user, const string& resource_)
    : session_id(user),
      resource(resource_),
      session(0),
      message_event_filter(0),
      impl(impl_)
{
    JID jid(impl->getXmpp()->jid());
    jid.setUsername(user);
    jid.setResource((resource == "*") ? "" : resource);

    session = new MessageSession(impl->getXmpp(), jid, (resource != "*"));
    session->registerMessageHandler(impl->getChatImpl());

    try {
        message_event_filter = new MessageEventFilter(session);
        message_event_filter->registerMessageEventHandler(impl->getChatImpl());
    }
    catch(...) {
        impl->getXmpp()->disposeMessageSession(session);

        throw;
    }
}

XMPPClient::ChatSession::~ChatSession()
{
    session->disposeMessageFilter(message_event_filter);

    impl->getXmpp()->disposeMessageSession(session);
}

/// XMPPClient::ChatImpl
XMPPClient::ChatImpl::ChatImpl(XMPPClient *client_, ClientImpl *impl_)
    : client(client_), impl(impl_)
{
}

XMPPClient::ChatImpl::~ChatImpl()
{
    disposeChatSessions();
}

bool XMPPClient::ChatImpl::sendChatMessage(const string& user, const string& message, const string& subject, const string& resource)
{
    if(message.empty()) {
        // do not let sending empty messages
        return true;
    }

    MutexGuard guard(chat_sessions_lock);

    ChatSession *chat_session = findChatSession(user);
    if(!resource.empty()) {
        if(chat_session) {
            if(chat_session->resource != resource) {
                delete chat_session;
                chat_session = 0;
            }
        }
    }

    if(!chat_session) {
        chat_session = new ChatSession(impl, user, resource);
        assert(chat_session->session_id == user);
        chat_sessions[chat_session->session_id] = chat_session;
    }

    chat_session->session->send(message, subject);
    return true;
}

bool XMPPClient::ChatImpl::sendChatMessageComposing(const string& user, const string& resource)
{
    MutexGuard guard(chat_sessions_lock);

    ChatSession *chat_session = findChatSession(user);
    if(resource.empty()) {
        if(chat_session) {
            chat_session->message_event_filter->raiseMessageEvent(MessageEventComposing);
            return true;
        }
    }
    else  {
        if(chat_session) {
            if(chat_session->resource != resource) {
                delete chat_session;
                chat_session = 0;
            }
        }

        if(!chat_session) {
            chat_session = new ChatSession(impl, user, resource);
            assert(chat_session->session_id == user);
            chat_sessions[chat_session->session_id] = chat_session;
        }

        chat_session->message_event_filter->raiseMessageEvent(MessageEventComposing);
        return true;
    }

    return false;
}

bool XMPPClient::ChatImpl::sendChatMessageDelivered(const string& user, const string& resource)
{
    MutexGuard guard(chat_sessions_lock);

    ChatSession *chat_session = findChatSession(user);
    if(resource.empty()) {
        if(chat_session) {
            chat_session->message_event_filter->raiseMessageEvent(MessageEventDelivered);
            return true;
        }
    }
    else  {
        if(chat_session) {
            if(chat_session->resource != resource) {
                delete chat_session;
                chat_session = 0;
            }
        }

        if(!chat_session) {
            chat_session = new ChatSession(impl, user, resource);
            assert(chat_session->session_id == user);
            chat_sessions[chat_session->session_id] = chat_session;
        }

        chat_session->message_event_filter->raiseMessageEvent(MessageEventDelivered);
        return true;
    }

    return false;
}

void XMPPClient::ChatImpl::handlePrivateChatMessage(const string& user, const string& room, const Message& message)
{
    const string& body = message.body();

    if(body.empty()) {
        // do not raise XMPPClient::onChatMessage() on empty messages
        // NOTE: this behavior was removed from the latest version (1.0.9)
        // when triggered after MessageEventHandler::handleMessageEvent()
        return;
    }

    const DelayedDelivery *dd = message.when();
    // FIXME: should create sessions for delayed messages?

    {
        MutexGuard guard(chat_sessions_lock);

        ChatSession *chat_session = findChatSession(user);
        if(!chat_session) {
            chat_session = new ChatSession(impl, user);
            chat_sessions[chat_session->session_id] = chat_session;
        }
        assert(chat_session->session_id == user);
    }

    client->onChatMessage(user, room,
                          body, message.subject(),
                          (dd ? dd->stamp().c_str() : 0));
}

void XMPPClient::ChatImpl::handleChatSession(MessageSession *session)
{
    // WARN: can only dispose an old MessageSession with ClientBase::disposeMessageSession()
    // WARN: cannot dispose the current MessageSession here

    MutexGuard guard(chat_sessions_lock);

    const string& session_id = session->target().username();
    ChatSession *chat_session = findChatSession(session_id);
    if(chat_session) {
        delete chat_session;
    }

    chat_sessions[session_id] = new ChatSession(impl, session);
}

void XMPPClient::ChatImpl::handleChatMessageEvent(const JID& from, MessageEventType event)
{
    if(MessageEventDelivered & event) {
        client->onChatMessageDelivered(from.username(), from.resource());
    }
    else if(MessageEventComposing & event) {
        client->onChatMessageComposing(from.username(), from.resource());
    }

#if 0
    MutexGuard guard(chat_sessions_lock);

    const string& session_id = from.username();
    ChatSession *chat_session = findChatSession(session_id);
    if(chat_session) {
        if(MessageEventDelivered & event) {
            client->onChatMessageDelivered(session_id, from.resource());
        }
        else if(MessageEventComposing & event) {
            client->onChatMessageComposing(session_id, from.resource());
        }
    }
#endif

    // MessageEventOffline
    // MessageEventDisplayed
    // MessageEventInvalid
    // MessageEventCancel
}

void XMPPClient::ChatImpl::disposeChatSessions()
{
    MutexGuard guard(chat_sessions_lock);

    ChatSessions::const_iterator it = chat_sessions.begin();

    for(; it != chat_sessions.end(); ++it) {
        delete it->second;
    }
    chat_sessions.clear();
}

XMPPClient::ChatSession* XMPPClient::ChatImpl::findChatSession(const string& id)
{
    ChatSessions::const_iterator it = chat_sessions.find(id);

    if(it != chat_sessions.end()) {
        assert(it->second->session_id == id);
        return it->second;
    }
    return 0;
}

// MessageSessionHandler
void XMPPClient::ChatImpl::handleMessageSession(MessageSession *session)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> ChatImpl::handleMessageSession(): "
              "session=%p from='%s'\n",
              session, session->target().full().c_str());
#endif // _DEBUG

    // WARN: can only dispose an old MessageSession with ClientBase::disposeMessageSession()
    // WARN: cannot dispose the current MessageSession here

    handleChatSession(session);
}

// MessageHandler
void XMPPClient::ChatImpl::handleMessage(const Message& message, MessageSession *session)
{
    const string& body = message.body();

    if(body.empty()) {
        // do not raise XMPPClient::onChatMessage() on empty messages
        // NOTE: this behavior was removed from the latest version (1.0.9)
        // when triggered after MessageEventHandler::handleMessageEvent()
        return;
    }

    const JID& target = session->target();
    const DelayedDelivery *dd = message.when();

#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> ChatImpl::handleMessage(): "
              "session=%p from='%s' timestamp='%s'\n",
              session, target.full().c_str(),
              (dd ? dd->stamp().c_str() : "n/a"));
#endif // _DEBUG

    client->onChatMessage(target.username(), target.resource(),
                          body, message.subject(),
                          (dd ? dd->stamp().c_str() : 0));
}

// MessageEventHandler
void XMPPClient::ChatImpl::handleMessageEvent(const JID& from, MessageEventType event)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> ChatImpl::handleMessageEvent(): "
              "from='%s' event=%d\n",
              from.full().c_str(), (int)event);
#endif // _DEBUG

    handleChatMessageEvent(from, event);
}

/// XMPPClient::GroupChatSession
XMPPClient::GroupChatSession::GroupChatSession(ClientImpl *impl_, const string& group)
    : session_id(group),
      is_joined(false), room(0),
      creation_state(CREATION_STATE_NONE),
      impl(impl_)
{
    JID nick(group + "@" + impl->getClient()->getConfig().groupchat_server
             + "/" + impl->getXmpp()->jid().username());

#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatSession(): "
              "room='%s'\n", nick.full().c_str());
#endif // _DEBUG

    room = new MUCRoom(impl->getXmpp(), nick,
                       impl->getGroupChatImpl(), impl->getGroupChatImpl());
}

XMPPClient::GroupChatSession::~GroupChatSession()
{
    if(is_joined) {
        is_joined = false;
        room->leave();
    }

    delete room;
}

/// XMPPClient::GroupChatImpl
XMPPClient::GroupChatImpl::GroupChatImpl(XMPPClient *client_, ClientImpl *impl_)
    : MUCInvitationHandler(impl_->getXmpp()),
      client(client_), impl(impl_)
{
}

XMPPClient::GroupChatImpl::~GroupChatImpl()
{
    disposeGroupChatSessions();
}

#ifdef XMPP_CLIENT_INVITE_DECLINE_ENABLE
bool XMPPClient::GroupChatImpl::declineGroupChatInvitation(const string& group, const string& user, const string& reason)
{
    string room = group + "@" + impl->getClient()->getConfig().groupchat_server;

    JID jid(impl->getXmpp()->jid());
    jid.setUsername(user);

    // FIXME: TODO: is something wrong with jid.bareJID()?
    Message *message = MUCRoom::declineInvitation(room, jid.bareJID(), reason);
    impl->getXmpp()->send(*message);
    delete message;

    return true;
}
#endif // XMPP_CLIENT_INVITE_DECLINE_ENABLE

bool XMPPClient::GroupChatImpl::beginGroupChat(const string& group, const string& passwd,
                                               int history_messages, const string& history_since)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session) {
        return chat_session->is_joined;
    }

    chat_session = new GroupChatSession(impl, group);
    assert(chat_session->session_id == group);
    chat_sessions[chat_session->session_id] = chat_session;

    if(!passwd.empty()) {
        chat_session->room->setPassword(passwd);
    }

    if(history_messages >= 0) {
        chat_session->room->setRequestHistory(history_messages, MUCRoom::HistoryMaxStanzas);
    }
    else if(!history_since.empty()) {
        chat_session->room->setRequestHistory(history_since);
    }

    chat_session->room->join();
    chat_session->is_joined = true;

    return true;
}

bool XMPPClient::GroupChatImpl::endGroupChat(const string& group, const string& reason)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session) {
        if(chat_session->is_joined) {
            chat_session->is_joined = false;
            chat_session->room->leave(reason);
        }

        delete chat_session;
        chat_sessions.erase(group);

        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::destroyGroupChat(const string& group, const string& reason)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->is_joined = false;
        chat_session->room->destroy(reason);

        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::configureGroupChat(const string& group, const GroupChatConfig *config_)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        if(!config_) {
            if(chat_session->creation_state == GroupChatSession::CREATION_STATE_PENDING) {
                chat_session->room->acknowledgeInstantRoom();

                return true;
            }
        }
        else {
            config = *config_;
            chat_session->room->requestRoomConfig();

            return true;
        }
    }

    return false;
}

bool XMPPClient::GroupChatImpl::cancelGroupChatCreation(const string& group)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->cancelRoomCreation();
        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::setGroupChatSubject(const string& group, const string& subject)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->setSubject(subject);
        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::sendGroupChatMessage(const string& group, const string& message)
{
    if(message.empty()) {
        // do not let sending empty messages
        return true;
    }

    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->send(message);
        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::inviteToGroupChat(const string& group, const string& user, const string& reason)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        JID jid(impl->getXmpp()->jid());
        jid.setUsername(user);

        chat_session->room->invite(jid.bareJID(), reason);
        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::kickFromGroupChat(const string& group, const string& user, const string& reason)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->kick(user, reason);
        return true;
    }

    return false;
}

#ifdef XMPP_CLIENT_BAN_ENABLE
bool XMPPClient::GroupChatImpl::banFromGroupChat(const string& group, const string& user, const string& reason)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->ban(user, reason);
        return true;
    }

    return false;
}

bool XMPPClient::GroupChatImpl::unbanFromGroupChat(const string& group, const string& user)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->setAffiliation(user, AffiliationNone, "");
        return true;
    }

    return false;
}
#endif // XMPP_CLIENT_BAN_ENABLE

bool XMPPClient::GroupChatImpl::listGroupChatUsers(const string& group)
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(group);
    if(chat_session && chat_session->is_joined) {
        chat_session->room->getRoomItems();
        return true;
    }

    return false;
}

XMPPClient::GroupChatSession* XMPPClient::GroupChatImpl::findGroupChatSession(const string& id)
{
    GroupChatSessions::const_iterator it = chat_sessions.find(id);

    if(it != chat_sessions.end()) {
        assert(it->second->session_id == id);
        return it->second;
    }
    return 0;
}

void XMPPClient::GroupChatImpl::disposeGroupChatSessions()
{
    MutexGuard guard(chat_sessions_lock);

    GroupChatSessions::const_iterator it = chat_sessions.begin();

    for(; it != chat_sessions.end(); ++it) {
        delete it->second;
    }
    chat_sessions.clear();
}

// MUCRoomHandler
void XMPPClient::GroupChatImpl::handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence& presence)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCParticipantPresence(): "
              "group='%s' presence=%d nick='%s' JID='%s' reason='%s' status='%s'\n",
              room->name().c_str(), (int)presence.presence(),
              (participant.nick ? participant.nick->full().c_str() : "n/a"),
              (participant.jid ? participant.jid->full().c_str() : "n/a"),
              participant.reason.c_str(), participant.status.c_str());
#endif // _DEBUG

    const string& reason =
        participant.reason.empty() ?
        participant.status : participant.reason;

    switch(presence.presence()) {
    case Presence::Available:
    case Presence::Chat:
        client->onGroupChatUserPresence(room->name(), participant.nick->resource(),
                                        true, reason.c_str(), participant.flags);
        break;

    case Presence::Away:
    case Presence::DND:
    case Presence::XA:
    case Presence::Unavailable:
        client->onGroupChatUserPresence(room->name(), participant.nick->resource(),
                                        false, reason.c_str(), participant.flags);
        break;

    case Presence::Probe:
    case Presence::Error:
    case Presence::Invalid:
    default:
        break;
    }
}

void XMPPClient::GroupChatImpl::handleMUCMessage(MUCRoom *room, const Message& message, bool priv)
{
    const string& user = message.from().resource();
    const DelayedDelivery *dd = message.when();

#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCMessage(): "
              "group='%s' from='%s' nick='%s' private=%s timestamp='%s'\n",
              room->name().c_str(), message.from().full().c_str(),
              user.c_str(), (priv ? "yes" : "no"),
              (dd ? dd->stamp().c_str() : "n/a"));
#endif // _DEBUG

    if(priv) {
        impl->getChatImpl()->handlePrivateChatMessage(user, room->name(), message);
    }
    else {
        client->onGroupChatMessage(room->name(), user, message.body(),
                                   (dd ? dd->stamp().c_str() : 0));
    }
}

bool XMPPClient::GroupChatImpl::handleMUCRoomCreation(MUCRoom *room)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCRoomCreation(): "
              "group='%s'\n", room->name().c_str());
#endif // _DEBUG

    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(room->name());
    if(chat_session) {
        assert(chat_session->room == room);

        if(client->onGroupChatCreation(room->name()) == true) {
            chat_session->creation_state = GroupChatSession::CREATION_STATE_COMPLETE;
            return true;
        }
        else {
            chat_session->creation_state = GroupChatSession::CREATION_STATE_PENDING;
        }
    }

    return false;
}

void XMPPClient::GroupChatImpl::handleMUCSubject(MUCRoom *room, const string& nick, const string& subject)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCSubject(): "
              "group='%s' nick='%s' subject='%s'\n",
              room->name().c_str(), nick.c_str(), subject.c_str());
#endif // _DEBUG

    if(!nick.empty()) {
        client->onGroupChatSubject(room->name(), nick, subject);
    }
}

void XMPPClient::GroupChatImpl::handleMUCInviteDecline(MUCRoom *room, const JID& invitee, const string& reason)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCInviteDecline(): "
              "group='%s' invitee='%s' reason='%s'\n",
              room->name().c_str(), invitee.full().c_str(), reason.c_str());
#endif // _DEBUG

    client->onGroupChatInviteDecline(room->name(), invitee.username(), reason);
}

void XMPPClient::GroupChatImpl::handleMUCError(MUCRoom *room, StanzaError error)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCError(): "
              "group='%s' error=%d\n",
              room->name().c_str(), (int)error);
#endif // _DEBUG

    client->onGroupChatError(room->name(), error);
}

#ifdef _DEBUG
static void g_show_room_data_form_fields(const DataFormFieldContainer::FieldList& fields)
{
    DataFormFieldContainer::FieldList::const_iterator it;
    const DataFormField *field;

    for(it = fields.begin(); it != fields.end(); ++it) {
        field = *it;
        ::fprintf(stderr, "[name='%s' required=%s label='%s' description='%s' type=%d ",
                  field->name().c_str(), (field->required() ? "yes" : "no"),
                  field->label().c_str(), field->description().c_str(), (int)field->type());

        if(field->type() == DataFormField::TypeTextMulti) {
            ::fprintf(stderr, "values=[");

            const StringList& values = field->values();
            StringList::const_iterator it;

            for(it = values.begin(); it != values.end(); ++it) {
                ::fprintf(stderr, "'%s' ", it->c_str());
            }

            ::fprintf(stderr, "]] ");
        }
        else {
            ::fprintf(stderr, "value='%s'] ", field->value().c_str());
        }
    }
}

static void g_show_room_data_form(const DataForm& form)
{
    ::fprintf(stderr, "title='%s' type=%d instructions=[",
              form.title().c_str(), (int)form.type());

    StringList::const_iterator it;
    for(it = form.instructions().begin(); it != form.instructions().end(); ++it) {
        ::fprintf(stderr, "'%s' ", it->c_str());
    }

    ::fprintf(stderr, "] items=[");

    const DataForm::ItemList& items = form.items();
    DataForm::ItemList::const_iterator it_item;

    for(it_item = items.begin(); it_item != items.end(); ++it_item) {
        g_show_room_data_form_fields((*it_item)->fields());
    }

    ::fprintf(stderr, "] fields=[");

    g_show_room_data_form_fields(form.fields());

    ::fprintf(stderr, "]");
}
#endif // _DEBUG

void XMPPClient::GroupChatImpl::handleMUCInfo(MUCRoom *room, int features, const string& name, const DataForm *form)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCInfo(): "
              "group='%s' features=%d name='%s' form=[",
              room->name().c_str(), features, name.c_str());

    if(form) {
        g_show_room_data_form(*form);
    }
    else {
        ::fprintf(stderr, "n/a");
    }

    ::fprintf(stderr, "]\n");
#endif // _DEBUG
}

#ifdef _DEBUG
static void g_show_room_items(const Disco::ItemList& items)
{
    Disco::ItemList::const_iterator it;
    gloox::Disco::Item *item;

    for(it = items.begin(); it != items.end(); ++it) {
        item = *it;
        ::fprintf(stderr, "[JID='%s' name='%s' node='%s'] ",
                  item->jid().full().c_str(), item->name().c_str(), item->node().c_str());
    }
}
#endif // _DEBUG

void XMPPClient::GroupChatImpl::handleMUCItems(MUCRoom *room, const Disco::ItemList& items)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCItems(): "
              "group='%s' items=[", room->name().c_str());

    g_show_room_items(items);

    ::fprintf(stderr, "]\n");
#endif // _DEBUG

    StringList users;
    Disco::ItemList::const_iterator it;

    for(it = items.begin(); it != items.end(); ++it) {
        users.push_back((*it)->name());
    }

    client->onGroupChatUsersList(room->name(), users);
}

// MUCInvitationHandler
void XMPPClient::GroupChatImpl::handleMUCInvitation(const JID& room, const JID& from, const string& reason,
                                                    const string& body, const string& password, bool cont, const string& thread)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCInvitation(): "
              "group='%s' from='%s' reason='%s' body='%s' passwd='%s' cont=%s thread='%s'\n",
              room.full().c_str(), from.full().c_str(), reason.c_str(),
              body.c_str(), password.c_str(), cont ? "yes" : "no", thread.c_str());
#endif // _DEBUG

    if(!client->onGroupChatInvite(room.username(), from.username(), reason, password)) {
#ifdef XMPP_CLIENT_INVITE_DECLINE_ENABLE
        // decline invitation
        Message *message = MUCRoom::declineInvitation(room, from);
        impl->getXmpp()->send(*message);
        delete message;
#endif // XMPP_CLIENT_INVITE_DECLINE_ENABLE
    }
}

#ifdef _DEBUG
static void g_show_config_room_items(const MUCListItemList& items)
{
    MUCListItemList::const_iterator it;
    for(it = items.begin(); it != items.end(); ++it) {
        ::fprintf(stderr, "[JID='%s' nick='%s' affiliation=%d role=%d reason='%s'] ",
                  it->jid().full().c_str(), it->nick().c_str(),
                  (int)it->affiliation(), (int)it->role(), it->reason().c_str());
    }
}
#endif // _DEBUG

// MUCRoomConfigHandler
void XMPPClient::GroupChatImpl::handleMUCConfigList(MUCRoom* room, const MUCListItemList& items, MUCOperation operation)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCConfigList(): "
              "group='%s' operation=%d items=[\n",
              room->name().c_str(), (int)operation);

    g_show_config_room_items(items);

    ::fprintf(stderr, "]\n");
#endif // _DEBUG
}

static DataForm* g_configure_room_dataform(const XMPPClient::GroupChatConfig& config, const DataForm& form)
{
    DataForm *new_form = new DataForm(TypeSubmit, form.instructions(), form.title());
    bool status = false;

    do {
        if(!form.hasField("muc#roomconfig_roomname")) break;
        new_form->addField(DataFormField::TypeTextSingle, "muc#roomconfig_roomname",
                           config.title);

        if(!form.hasField("muc#roomconfig_roomdesc")) break;
        new_form->addField(DataFormField::TypeTextSingle, "muc#roomconfig_roomdesc",
                           config.description);

        if(!form.hasField("muc#roomconfig_persistentroom")) break;
        new_form->addField(DataFormField::TypeBoolean, "muc#roomconfig_persistentroom",
                           config.is_persistent ? "1" : "0");

        if(!(form.hasField("muc#roomconfig_passwordprotectedroom")
             && form.hasField("muc#roomconfig_roomsecret"))) break;
        new_form->addField(DataFormField::TypeBoolean, "muc#roomconfig_passwordprotectedroom",
                           config.passwd.empty() ? "0" : "1");
        new_form->addField(DataFormField::TypeTextPrivate, "muc#roomconfig_roomsecret",
                           config.passwd);

        if(!form.hasField("muc#roomconfig_maxusers")) break;
        new_form->addField(DataFormField::TypeListSingle, "muc#roomconfig_maxusers",
                           config.max_number_users);

        if(!form.hasField("muc#roomconfig_membersonly")) break;
        new_form->addField(DataFormField::TypeBoolean, "muc#roomconfig_membersonly",
                           config.is_members_only ? "1" : "0");

        if(!form.hasField("muc#roomconfig_changesubject")) break;
        new_form->addField(DataFormField::TypeBoolean, "muc#roomconfig_changesubject",
                           config.is_user_subject_change_allowed ? "1" : "0");

        if(!form.hasField("muc#roomconfig_allowinvites")) break;
        new_form->addField(DataFormField::TypeBoolean, "muc#roomconfig_allowinvites",
                           config.is_user_invite_send_allowed ? "1" : "0");

        if(!form.hasField("muc#roomconfig_allowvoicerequests")) break;
        new_form->addField(DataFormField::TypeBoolean, "muc#roomconfig_allowvoicerequests",
                           config.is_user_send_voice_request_allowed ? "1" : "0");

        status = true;
    }
    while(0);

    if(!status) {
        delete new_form;
        new_form = 0;
    }
    return new_form;
}

void XMPPClient::GroupChatImpl::handleMUCConfigForm(MUCRoom* room, const DataForm& form)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCConfigForm(): "
              "group='%s' form=[", room->name().c_str());

    g_show_room_data_form(form);

    ::fprintf(stderr, "]\n");
#endif // _DEBUG

    MutexGuard guard(chat_sessions_lock);

    GroupChatSession *chat_session = findGroupChatSession(room->name());
    if(chat_session) {
        assert(chat_session->room == room);

        DataForm *new_form = g_configure_room_dataform(config, form);
        if(new_form) {
            room->setRoomConfig(new_form);
        }

        // FIXME: should be called in XMPPClientGroupChatImpl::handleMUCConfigResult() callback
        if(chat_session->creation_state == GroupChatSession::CREATION_STATE_PENDING) {
            chat_session->creation_state = GroupChatSession::CREATION_STATE_COMPLETE;
            client->onGroupChatCreate(room->name(), (new_form != 0));
        }
    }
}

void XMPPClient::GroupChatImpl::handleMUCConfigResult(MUCRoom* room, bool success, MUCOperation operation)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCConfigResult(): "
              "group='%s' success=%s operation=%d\n",
              room->name().c_str(), success ? "yes" : "no", (int)operation);
#endif // _DEBUG

    switch(operation) {
    case SetRNone:
        client->onGroupChatKickResult(room->name(), success);
        break;
#ifdef XMPP_CLIENT_BAN_ENABLE
    case SetOutcast:
        client->onGroupChatBanResult(room->name(), success);
        break;
    case SetANone:
        client->onGroupChatUnbanResult(room->name(), success);
        break;
#endif // XMPP_CLIENT_BAN_ENABLE
    case CreateInstantRoom:
    {
        MutexGuard guard(chat_sessions_lock);

        GroupChatSession *chat_session = findGroupChatSession(room->name());
        if(chat_session) {
            assert(chat_session->room == room);

            chat_session->creation_state = GroupChatSession::CREATION_STATE_COMPLETE;
            client->onGroupChatCreate(room->name(), success);
        }
    }
    break;
    case DestroyRoom:
        client->onGroupChatDestroy(room->name(), success);
        break;

    case CancelRoomCreation:
    case RequestRoomConfig:
    case SendRoomConfig:
        // FIXME: do we need signal error cases? (sometimes it doesn't event happen)
        //if(!success) {
        //    client->onGroupChatOperationError(room->name(), operation);
        //}
#ifndef XMPP_CLIENT_BAN_ENABLE
    case SetOutcast:
    case SetANone:
#endif // XMPP_CLIENT_BAN_ENABLE
    default:
        break;
    }
}

void XMPPClient::GroupChatImpl::handleMUCRequest(MUCRoom* room, const DataForm& form)
{
#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> GroupChatImpl::handleMUCRequest(): "
              "group='%s' form=[", room->name().c_str());

    g_show_room_data_form(form);

    ::fprintf(stderr, "]\n");
#endif // _DEBUG
}

/// XMPPClient
XMPPClient::Error::Error(const string& error)
    : runtime_error(error)
{
}

XMPPClient::Error::Error(const char *error)
    : runtime_error(error)
{
}

XMPPClient::Error::Error(int errnum)
    : runtime_error(strerror(errnum))
{
}

static string g_extract_server_domain(const string& jid)
{
    string server = JID(jid).server();

    size_t pos = server.find('.');
    if(pos != string::npos) {
        return server.substr(pos + 1);
    }

    return server;
}

XMPPClient::Config::Config(const string& jid_, const string& passwd_)
    : jid(jid_), passwd(passwd_),
      server(""), port(5222),
      tls_policy(TLSOptional),
      groupchat_server("conference." + g_extract_server_domain(jid)),
      recv_timeout(-1)
{
}

XMPPClient::Config::Config()
    : jid("user@host.domain"), passwd("password"),
      server(""), port(5222),
      tls_policy(TLSOptional),
      groupchat_server("conference." + g_extract_server_domain(jid)),
      recv_timeout(-1)
{
}

XMPPClient::Config::Config(const Config& config)
    : jid(config.jid), passwd(config.passwd),
      server(config.server), port(config.port),
      tls_policy(config.tls_policy),
      ca_certs(config.ca_certs),
      groupchat_server(config.groupchat_server),
      recv_timeout(config.recv_timeout)
{
}

const XMPPClient::Config& XMPPClient::Config::operator=(const Config& config)
{
    if(this != &config) {
        jid = config.jid;
        passwd = config.passwd;
        server = config.server;
        port = config.port;
        tls_policy = config.tls_policy;
        ca_certs = config.ca_certs;
        groupchat_server = config.groupchat_server;
        recv_timeout = config.recv_timeout;
    }

    return *this;
}

#ifdef _DEBUG
static ostream& operator<<(ostream& ost, const XMPPClient::Config& config)
{
    ost << "Client configuration: JID='" << config.jid << "'"
        << " passwd='" <<  config.passwd << "'"
        << " server='" <<  config.server << "'"
        << " port=" << config.port
        << " tls_policy=" <<  config.tls_policy
        << " groupchat_server='" <<  config.groupchat_server << "'"
        << " recv_timeout=" << config.recv_timeout;

    return ost;
}
#endif // _DEBUG

XMPPClient::XMPPClient(const Config& config_)
    : config(config_), impl(0),
      is_connected(false), is_running(false),
      recv_timeout(-1)
{
#ifdef _DEBUG
    cerr << ">>> DEBUG >>> XMPP_CLIENT >>> " << config << endl;
#endif // _DEBUG

    if(config.recv_timeout >= 0) {
        recv_timeout = (config.recv_timeout * 1000);
    }

    impl = new ClientImpl(this, config);
}

XMPPClient::~XMPPClient()
{
    delete impl;
}

/// connection methods
bool XMPPClient::connect(bool start_thread)
{
    if((start_thread && is_running) || is_connected) {
        return true;
    }

    is_connected = impl->getXmpp()->connect(false);
    if(!is_connected) {
        return false;
    }

    if(start_thread) {
        is_running = true;

        if(::pthread_create(&event_loop_thread, 0, XMPPClient::event_loop, this) != 0) {
            impl->getXmpp()->disconnect();
            is_connected = false;
            is_running = false;
        }
    }

    return is_connected;
}

void XMPPClient::disconnect()
{
    impl->getXmpp()->disconnect();

    // always wait for event_loop_thread() to finish
    is_running = false;
    ::pthread_join(event_loop_thread, 0);

    impl->getGroupChatImpl()->disposeGroupChatSessions();
    impl->getChatImpl()->disposeChatSessions();
}

void* XMPPClient::event_loop(void *data)
{
    XMPPClient* client = static_cast<XMPPClient*>(data);

#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> XMPPClient::onClientStart()\n");
#endif // _DEBUG
    client->onClientStart();

    while(client->is_running) {
        if(!client->internalUpdate(client->recv_timeout)) {
            client->is_running = false;
        }
        sched_yield();
    }

#ifdef _DEBUG
    ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> XMPPClient::onClientStop()\n");
#endif // _DEBUG
    client->onClientStop();

    return 0;
}

bool XMPPClient::update(int timeout /* milliseconds */)
{
    if(is_running) {
        return true;
    }

    return internalUpdate((timeout == -1) ? timeout : (timeout * 1000));
}

bool XMPPClient::internalUpdate(int timeout /* microseconds */)
{
    ConnectionError error = impl->getXmpp()->recv(timeout);
    ConnectionState state = impl->getXmpp()->state();
    bool retcode = true;

    if(error == ConnNoError) {
        if(StateDisconnected == state) {
            is_connected = false;
            retcode = false;
        }
        else if(is_running == false) {
            // NOTE: it appears there is a broken connection that timeouts
            is_connected = false;
            retcode = false;
        }
    }
    else {
        switch(error) {
        case ConnNotConnected:
        case ConnUserDisconnected:
            is_connected = false;
            retcode = false;
            break;

        default:
            if(StateDisconnected == state) {
                is_connected = false;
                retcode = false;
                break;
            }

#ifdef _DEBUG
            ::fprintf(stderr, ">>> DEBUG >>> XMPP_CLIENT >>> XMPPClient::handleUpdateError(): "
                      "state=%d error=%d\n", (int)state, (int)error);
#endif // _DEBUG
            retcode = handleUpdateError(state, error);
            break;
        }
    }

    return retcode;
}

gloox::Client* XMPPClient::getXmpp() const
{
    return impl->getXmpp();
}

bool XMPPClient::handleUpdateError(ConnectionState state, ConnectionError error)
{
    (void)state, void(error);

    // treat any error as handled
    return true;
}

void XMPPClient::onClientStart()
{
    // do nothing
}

void XMPPClient::onClientStop()
{
    // do nothing
}

/// connection callbacks
void XMPPClient::onConnect()
{
    // do nothing
}

bool XMPPClient::onTlsConnect(const CertInfo& info)
{
    (void)info;

    // reject any certificate by default
    return false;
}

void XMPPClient::onDisconnect(ConnectionError error)
{
    (void)error;

    // do nothing
}

/// simple chat methods
bool XMPPClient::sendChatMessage(const string& user, const string& message, const string& subject, const string& resource)
{
    if(!is_connected) {
        return false;
    }

    return impl->getChatImpl()->sendChatMessage(user, message, subject, resource);
}

bool XMPPClient::sendChatMessageComposing(const string& user, const string& resource)
{
    if(!is_connected) {
        return false;
    }

    return impl->getChatImpl()->sendChatMessageComposing(user, resource);
}

bool XMPPClient::sendChatMessageDelivered(const string& user, const string& resource)
{
    if(!is_connected) {
        return false;
    }

    return impl->getChatImpl()->sendChatMessageDelivered(user, resource);
}

/// simple chat callbacks
void XMPPClient::onChatMessage(const string& user, const string& resource,
                               const string& message, const string& subject, const char *timestamp)
{
    (void)user, (void)resource, (void)message, (void)subject, (void)timestamp;

    // do nothing
}

void XMPPClient::onChatMessageComposing(const string& user, const string& resource)
{
    (void)user, (void)resource;

    // do nothing
}

void XMPPClient::onChatMessageDelivered(const string& user, const string& resource)
{
    (void)user, (void)resource;

    // do nothing
}

/// group chat methods
#ifdef XMPP_CLIENT_INVITE_DECLINE_ENABLE
bool XMPPClient::declineGroupChatInvitation(const string& group, const string& user, const string& reason)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->declineGroupChatInvitation(group, user, reason);
}
#endif // XMPP_CLIENT_INVITE_DECLINE_ENABLE

bool XMPPClient::beginGroupChat(const string& group, const string& passwd,
                                int history_messages, const string& history_since)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->beginGroupChat(group, passwd, history_messages, history_since);
}

bool XMPPClient::endGroupChat(const string& group, const string& reason)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->endGroupChat(group, reason);
}

bool XMPPClient::destroyGroupChat(const string& group, const string& reason)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->destroyGroupChat(group, reason);
}

const string XMPPClient::GroupChatConfig::CONF_TITLE                               = "";
const string XMPPClient::GroupChatConfig::CONF_DESCRIPTION                         = "";
const bool   XMPPClient::GroupChatConfig::CONF_IS_PERSISTENT                       = false;
const string XMPPClient::GroupChatConfig::CONF_PASSWD                              = "";
const string XMPPClient::GroupChatConfig::CONF_MAX_NUMBER_USERS                    = "200";
const bool   XMPPClient::GroupChatConfig::CONF_IS_MEMBERS_ONLY                     = false;
const bool   XMPPClient::GroupChatConfig::CONF_IS_USER_SUBJECT_CHANGE_ALLOWED      = true;
const bool   XMPPClient::GroupChatConfig::CONF_IS_USER_INVITE_SEND_ALLOWED         = false;
const bool   XMPPClient::GroupChatConfig::CONF_IS_USER_SEND_VOICE_REQUEST_ALLOWED  = true;

XMPPClient::GroupChatConfig::GroupChatConfig()
    : title(CONF_TITLE),
      description(CONF_DESCRIPTION),
      is_persistent(CONF_IS_PERSISTENT),
      passwd(CONF_PASSWD),
      max_number_users(CONF_MAX_NUMBER_USERS),
      is_members_only(CONF_IS_MEMBERS_ONLY),
      is_user_subject_change_allowed(CONF_IS_USER_SUBJECT_CHANGE_ALLOWED),
      is_user_invite_send_allowed(CONF_IS_USER_INVITE_SEND_ALLOWED),
      is_user_send_voice_request_allowed(CONF_IS_USER_SEND_VOICE_REQUEST_ALLOWED)
{
}

XMPPClient::GroupChatConfig::GroupChatConfig(const GroupChatConfig& config)
    : title(config.title),
      description(config.description),
      is_persistent(config.is_persistent),
      passwd(config.passwd),
      max_number_users(config.max_number_users),
      is_members_only(config.is_members_only),
      is_user_subject_change_allowed(config.is_user_subject_change_allowed),
      is_user_invite_send_allowed(config.is_user_invite_send_allowed),
      is_user_send_voice_request_allowed(config.is_user_send_voice_request_allowed)
{
}

const XMPPClient::GroupChatConfig& XMPPClient::GroupChatConfig::operator=(const GroupChatConfig& config)
{
    if(this != &config) {
        title = config.title;
        description = config.description;
        is_persistent = config.is_persistent;
        passwd = config.passwd;
        max_number_users = config.max_number_users;
        is_members_only = config.is_members_only;
        is_user_subject_change_allowed = config.is_user_subject_change_allowed;
        is_user_invite_send_allowed = config.is_user_invite_send_allowed;
        is_user_send_voice_request_allowed = config.is_user_send_voice_request_allowed;
    }

    return *this;
}

#ifdef _DEBUG
static ostream& operator<<(ostream& ost, const XMPPClient::GroupChatConfig& config)
{
    ost << "Group chat configuration parameters: title='" << config.title << "'"
        << " description='" << config.description << "'"
        << " is_persistent=" << config.is_persistent
        << " passwd='" <<  config.passwd << "'"
        << " max_number_users=" <<  config.max_number_users
        << " is_members_only=" << config.is_members_only
        << " is_user_subject_change_allowed=" <<  config.is_user_subject_change_allowed
        << " is_user_invite_send_allowed=" <<  config.is_user_invite_send_allowed
        << " is_user_send_voice_request_allowed=" <<  config.is_user_send_voice_request_allowed;

    return ost;
}
#endif // _DEBUG

bool XMPPClient::configureGroupChat(const string& group, const GroupChatConfig *config)
{
    if(!is_connected) {
        return false;
    }

#ifdef _DEBUG
    if(config) {
        cerr << ">>> DEBUG >>> XMPP_CLIENT >>> XMPPClient::configureGroupChat(): "
             << *config << endl;
    }
    else {
        cerr << ">>> DEBUG >>> XMPP_CLIENT >>> XMPPClient::configureGroupChat(): n/a"
             << endl;
    }
#endif // _DEBUG

    return impl->getGroupChatImpl()->configureGroupChat(group, config);
}

bool XMPPClient::cancelGroupChatCreation(const string& group)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->cancelGroupChatCreation(group);
}

bool XMPPClient::setGroupChatSubject(const string& group, const string& subject)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->setGroupChatSubject(group, subject);
}

bool XMPPClient::sendGroupChatMessage(const string& group, const string& message)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->sendGroupChatMessage(group, message);
}

bool XMPPClient::inviteToGroupChat(const string& group, const string& user, const string& reason)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->inviteToGroupChat(group, user, reason);
}

bool XMPPClient::kickFromGroupChat(const string& group, const string& user, const string& reason)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->kickFromGroupChat(group, user, reason);
}

#ifdef XMPP_CLIENT_BAN_ENABLE
bool XMPPClient::banFromGroupChat(const string& group, const string& user, const string& reason)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->banFromGroupChat(group, user, reason);
}

bool XMPPClient::unbanFromGroupChat(const string& group, const string& user)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->unbanFromGroupChat(group, user);
}
#endif // XMPP_CLIENT_BAN_ENABLE

bool XMPPClient::listGroupChatUsers(const string& group)
{
    if(!is_connected) {
        return false;
    }

    return impl->getGroupChatImpl()->listGroupChatUsers(group);
}

/// group chat callbacks
bool XMPPClient::onGroupChatCreation(const string& group)
{
    (void)group;

    // accept the default room configuration
    return true;
}

void XMPPClient::onGroupChatCreate(const string& group, bool success)
{
    (void)group, (void)success;

    // do nothing
}

void XMPPClient::onGroupChatDestroy(const string& group, bool success)
{
    (void)group, (void)success;

    // do nothing
}

void XMPPClient::onGroupChatSubject(const string& group, const string& user, const string& subject)
{
    (void)group, (void)user, (void)subject;

    // do nothing
}

void XMPPClient::onGroupChatMessage(const string& group, const string& user,
                                    const string& message, const char *timestamp)
{
    (void)group, (void)user, (void)message, (void)timestamp;

    // do nothing
}

bool XMPPClient::onGroupChatInvite(const string& group, const string& user,
                                   const string& reason, const string& passwd)
{
    (void)group, (void)user, (void)reason, (void)passwd;

    // decline invitation
    return false;
}

void XMPPClient::onGroupChatKickResult(const string& group, bool success)
{
    (void)group, (void)success;

    // do nothing
}

#ifdef XMPP_CLIENT_BAN_ENABLE
void XMPPClient::onGroupChatBanResult(const string& group, bool success)
{
    (void)group, (void)success;

    // do nothing
}

void XMPPClient::onGroupChatUnbanResult(const string& group, bool success)
{
    (void)group, (void)success;

    // do nothing
}
#endif // XMPP_CLIENT_BAN_ENABLE

void XMPPClient::onGroupChatUserPresence(const string& group, const string& user,
                                         bool online, const string& reason, int flags)
{
    (void)group, (void)user, (void)online, (void)reason, (void)flags;

    // do nothing
}

// FIXME: do we need signal error cases? (sometimes it doesn't event happen)
//void XMPPClient::onGroupChatOperationError(const string& group, MUCOperation operation)
//{
//    // do nothing
//}

void XMPPClient::onGroupChatError(const string& group, StanzaError error)
{
    (void)group, (void)error;

    // do nothing
}

void XMPPClient::onGroupChatInviteDecline(const string& group, const string& user, const string& reason)
{
    (void)group, (void)user, (void)reason;

    // do nothing
}

void XMPPClient::onGroupChatUsersList(const string& group, const StringList& users)
{
    (void)group, (void)users;

    // do nothing
}
