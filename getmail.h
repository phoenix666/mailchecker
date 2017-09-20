#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <iostream>
#include <gtkmm/statusbar.h>
#include <glibmm/ustring.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};  
class MailMessage {
public:
	int mbIndex,m_size,b_size;
	char *buf;
	bool spam,deleteIt;
	MailMessage();
	~MailMessage();
	Glib::ustring Subject();
	Glib::ustring Header();
	Glib::ustring From();
	Glib::ustring Body();
	Glib::ustring Whole();
};
class MailBoxContents{
public:
	int mCount;
	int oldCount;
	MailMessage *messages;
	MailBoxContents();
	~MailBoxContents();
	int CheckMail(Gtk::Statusbar *stBar,const char *user,const char *pwd,const char *server,const char *port,const char *aliases,const char *blacklist,const char *whitelist);
	int WashMail(Gtk::Statusbar *stBar,const char *user,const char *pwd,const char *server,const char *port);
};