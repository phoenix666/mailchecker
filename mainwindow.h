#ifndef GTKMM_MAINWINDOW_H
#define GTKMM_MAINWINDOW_H

#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/box.h>
#include <gtkmm/listbox.h>
#include <gtkmm/textview.h>
#include <gtkmm/window.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/notebook.h>
#include <gtkmm/statusbar.h>
#include <glibmm/ustring.h>
#include <glibmm/ustring.h>
#include <glibmm/main.h>
#include <gtkmm/checkbutton.h>
#include <gdkmm/pixbuf.h>
#include "getmail.h"

class MainWindow : public Gtk::Window
{

public:
  MainWindow();
  virtual ~MainWindow();

protected:
  //Signal handlers:
  void on_button2_clicked();
  void on_button3_clicked();
  bool text_cursor_move(GdkEventMotion* motion_event);
  bool link_clicked(const Glib::RefPtr<Glib::Object>& event_object, GdkEvent* gdk_event, const Gtk::TextIter& iter);
  void on_mbl_select();
  bool on_timeout();
  void on_button_clicked();
  void on_page_switched(Widget* page, guint page_number);
  void load_data();
  void save_data();
  bool firstCheck = true;
  char *old_data = NULL;
  bool cursor_hand = false;
  
  //Member widgets:
  Gtk::VBox page1_main_box;
  Gtk::HBox page1_buttons_box;
  Gtk::ScrolledWindow tv_ScrolledWindow,ml_ScrolledWindow;
  Gtk::TextView MailView;
  Gtk::ListBox MailList;
  Gtk::Button getmail_button,washmail_button,showheader_button;
  //pthread_t check_mail_thread;
  Gtk::Notebook MainBook;
  Gtk::Entry eServer,ePort,eUsername,ePassword;
  Glib::RefPtr<Gtk::TextBuffer> alBuffer,blBuffer,wlBuffer;
public:
  Gtk::Statusbar InfoBar;
  Glib::RefPtr<Gtk::TextBuffer> tv_buffer;
  void RepopulateListBox();
};
class MH_Row : public Gtk::ListBoxRow{
	Gtk::Label l_From,l_Subject;
	Gtk::VBox main_box;
	Gtk::HBox line_box;
	MailMessage *message;
	Gtk::CheckButton spam_button,del_button;
	void on_button_clicked();
public:
	MH_Row(MailMessage *mes);
	~MH_Row();
	MailMessage *Message() {return message;};
	bool Spam() {return spam_button.get_active();}
	bool DeleteIt() {return del_button.get_active();}
};

#endif // GTKMM_MAINWINDOW_H