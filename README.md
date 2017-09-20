# mailchecker
It's a program I needed for myself when I switched from Windows to Ubuntu. Inspired by MailWasher for Windows which does not have Linux version.

What it does: it checks one pop3 mail box for mail every 10 minutes and plays sound if there is new mail that does not look as spam. It allows you to view first several lines of the mails and headers if you want to see them. You can click links if there are any in the mail. It marks as spam everything that is Chinese, or if any of the header IPs are found in SpamCop black list, or if email headers contain piece of text that you explicitly specified in the black list. It does not shrink into tray, you have to keep it active.

Warning: This is a piece of shit code I wrote gradually starting from Gui Test example. I never wrote anything for Gtk or Ubuntu before. Vague documentation on Gtkmm did not particularly help, so I may have done a lot of things improperly. I literally had to guess a lot of things by "try-fail until it works". Mail retrieving and parsing part is also a piece of shit, not parsing headers and MIME parts as it should, just enough to get what I usually need. It may segfault on some mail formats yhat I did not happen to run into, but it works stable for everything I got so far. Many things are hard-coded instead of being configured at run-time as they should. I do not know if it's gonna work with gmail, I only needed it for pop3 hosted on my own box. There is no support of multiple mail boxes, I only needed one. And adding support of multiple boxes to this code can not be easy, you should rebuild the whole conception if you want multiple mail boxes. It does not have learning spam filter. I wanted to do it but realised I don't need it. So checking "S" checkbox does nothing.

I know this entire code is an example of how things SHOULD NOT BE DONE. But I did it just for myself and it does what I need. Since there is no alternatives to MailWasher under Ubuntu I'm publishing this code anyway, may be someone else can use it or make something better out of it.

You need to grab executable (MailChecker file) and vogel.wav, put them into any directory you want, run the executable and theoretically it should work under Ubuntu. If executable does not, I guess you can grab the source code files and compile it yourself. I used CodeLite for compilation. I used gtkmm-3.0 and curl libs. Curl lib needs to be specified explicitly to the liker.
