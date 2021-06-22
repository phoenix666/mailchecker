#include "getmail.h"
#include <glibmm/convert.h>
#include <netdb.h>

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */ 
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
    mem->memory[mem->size] = 0;
	
    return realsize;
}

int extract_numbers(MemoryStruct chunk,int* &numbers)
{
	char *str = (char*) chunk.memory;
	int k = 0;
	int nmode = 0;
	for(int i = 0;i<chunk.size;i++)
	{
		if(nmode==0 && str[i]>='0' && str[i]<='9') 
		{
			nmode = 1;
			k++;
		}
		else if(nmode==1 && (str[i]<'0' || str[i]>'9')) nmode = 0;
	}
	int n = 0;
	if(k>0)
	{
		numbers = new int[k];
		nmode = 0;
		char ts[100];
		k = 0;
		for(int i = 0;i<chunk.size;i++)
		{
			if(nmode==0 && str[i]>='0' && str[i]<='9') 
			{
				nmode = 1;
				ts[k++] = str[i];
			}
			else if(nmode==1)
			{
				if (str[i]<'0' || str[i]>'9') 
				{
					ts[k] = 0;
					numbers[n++] = atoi(ts);
					nmode = 0;
					k = 0;
				}
				else ts[k++] = str[i];
			}
		}
	}
	return(n);
}

MailBoxContents::MailBoxContents()
{
	mCount = 0;
	oldCount = 0;
	messages = NULL;
}
MailBoxContents::~MailBoxContents()
{
	if(messages!=NULL) delete[] messages;
}

int MailBoxContents::WashMail(Gtk::Statusbar *stBar,const char *user,const char *pwd,const char *server,const char *port)
{
  stBar->remove_all_messages();
  if(strlen(server)<5) 
  {
	  stBar->push("No connection data");
	  return(0);
  }
  int need_deleting = 0;
  for(int i = 0;i<mCount;i++) if(messages[i].deleteIt) need_deleting = 1;
  if(need_deleting==0)
  {
	  stBar->push("Nothing to delete");
	  return(0);
  }
  stBar->push("Reading list of messages...");
  while (gtk_events_pending ()) gtk_main_iteration ();
  CURL *curl;
  CURLcode res = CURLE_OK;
  
  struct MemoryStruct chunk;

  chunk.memory = (char*) malloc(1);  //crecerá según sea necesario con el realloc
  chunk.size = 0;
 
  curl = curl_easy_init();
  if(curl) {
    /* Set username and password */ 
    curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pwd);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 40);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
	char tstr[100];
	
	if(atoi(port)==995)	strcpy(tstr,"pop3s://");
	else strcpy(tstr,"pop3://");
	
	strcat(tstr,server);
	if(strlen(port)>0 && atoi(port)!=0) 
	{
		strcat(tstr,":");
		strcat(tstr,port);
	}
    curl_easy_setopt(curl, CURLOPT_URL, tstr);
 
    /* Perform the list */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
	{
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
	  need_deleting = 0;
	}
	else{
        int *numbers;
		int n = extract_numbers(chunk,numbers);
		if(n>0 && n%2==0)
		{
			if(mCount<=(n>>1))
			{
				int i;
				for(i = 0;i<mCount;i++) if(messages[i].m_size!=numbers[(i<<1)+1]) break;
				if(i<mCount) need_deleting = 0;
			}			
			delete(numbers);
		}
		else if(n==0) 
		{
			stBar->push("No messages in the mail box.");
			return 0;
		}
		else if(n%2!=0)
		{
			stBar->push("Weird error, count of numbers is odd");
			return 0;
		}
		if(need_deleting==0)
		{
			stBar->push("Can't delete, messages do not match, refresh the list.");
			return 0;
		}
		while (gtk_events_pending ()) gtk_main_iteration ();
	}

	if(need_deleting && mCount>0)
	{
		for(int i = 0;i<mCount;i++)
		{
			if(messages[i].deleteIt)
			{
				sprintf (tstr,"Deleting message #%i",messages[i].mbIndex);
				stBar->push(tstr);
				while (gtk_events_pending ()) gtk_main_iteration ();
				
				sprintf (tstr,"DELE %i",messages[i].mbIndex);
				curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, tstr);
				res = curl_easy_perform(curl);
			    if(res != CURLE_OK)
				{
				  fprintf(stderr, "curl_easy_perform() failed: %s\n",
						  curl_easy_strerror(res));
				}
			}
		}
	}
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "QUIT");
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
    }
  if(chunk.memory!=NULL) free(chunk.memory);
  while (gtk_events_pending ()) gtk_main_iteration ();
  return(1);
}
int *find_all_ips(char *str)
{
	int len = strlen(str);
	int count = 0;
	int mode = 0;
	int pcount = 0;
	int ncount = 0;
	for(int i = 0;i<=len;i++)
	{
		if((str[i]>='0' && str[i]<='9') || str[i]=='.') 
		{
			if(mode==0 && str[i]!='.') mode = 1;
			else if(mode == 1)
			{
				if(str[i]=='.')
				{
					ncount = 0;
					if(str[i-1]!='.') pcount++;
					else{ mode = 0;pcount = 0;}
				}
				else{
					ncount++;
					if(ncount>3) {mode = 0;ncount = 0;}
				}
			}
		}
		else{
			mode = 0;
			if(pcount==3 && str[i-1]!='.') count++;
			pcount = 0;
		}
	}
	if(count>0)
	{
		int *positions = new int[count*2+1];
		mode = 0;
		int k = 0;
		pcount = 0;
		ncount = 0;
		for(int i = 0;i<=len;i++)
		{
			if((str[i]>='0' && str[i]<='9') || str[i]=='.') 
			{
				if(mode==0 && str[i]!='.') {mode = 1;positions[k] = i;}
				else if(mode == 1)
				{
					if(str[i]=='.')
					{
						ncount = 0;
						if(str[i-1]!='.') pcount++;
						else{ mode = 0;pcount = 0;}
					}
					else{
						ncount++;
						if(ncount>3) {mode = 0;ncount = 0;}
					}
				}
			}
			else{
				mode = 0;
				if(pcount==3 && str[i-1]!='.') 
				{
					positions[k+1] = i-positions[k];
					k += 2;
				}
				pcount = 0;
			}
		}
		positions[k] = -1;
		return(positions);
	}
	else return(NULL);
}
void reverse_string(char *str)
{
	int len = strlen(str);
	if(len>0)
	{
		for(int i = 0;i<len>>1;i++)
		{
			char t = str[i];
			str[i] = str[len-i-1];
			str[len-i-1] = t;
		}
	}
}
void reverse_ip_string(char *str)
{
	int len = strlen(str);
	if(len>0)
	{
		char *t = new char[len+1];
		int k = 0;
		for(int i = len-1;i>=0;i--)
		{
			if(str[i]=='.')
			{
				int j = i+1;
				while(str[j]!='.' && str[j]!=0) t[k++] = str[j++];
				t[k++] = '.';
			}
		}
		int j = 0;
		while(str[j]!='.' && k<len) t[k++] = str[j++];
		t[len] = 0;
		strcpy(str,t);
		delete(t);
	}
}
bool check_presense(const char *buf,const char *what)
{
	char *t = new char[strlen(what)+1];
	int i;
	for(i = 0;what[i]!=0;i++) t[i] = (char) toupper(what[i]);
	t[i] = 0;
	int k = 1;
	i = 0;
	while(t[i]!=0)
	{
		if(t[i]=='\n') k++;
		i++;
	}
	char **p = new char*[k];
	p[0] = t; 
	k = 1;
	i = 0;
	while(t[i]!=0)
	{
		if(t[i]=='\n')
		{
			t[i] = 0;
			p[k] = t+i+1;
			k++;
		}
		i++;
	}
	bool res = false;
	char *tb = new char[strlen(buf)+1];
	for(i = 0;buf[i]!=0;i++) tb[i] = (char) toupper(buf[i]);
	tb[i] = 0;
	for(i = 0;i<k;i++)
	{
		if(strlen(p[i])>2)
		{
			if(strstr(tb,p[i])!=NULL) 
			{
				res = true;
				break;
			}
		}
	}
	delete(tb);
	delete(p);
	delete(t);
	return(res);
}
int MailBoxContents::CheckMail(Gtk::Statusbar *stBar,const char *user,const char *pwd,const char *server,const char *port,const char *aliases,const char *blacklist,const char *whitelist)
{
  stBar->remove_all_messages();
  if(strlen(server)<5) 
  {
	  stBar->push("No connection data");
	  return(0);
  }
  stBar->push("Trying to receive list of messages");
  while (gtk_events_pending ()) gtk_main_iteration ();
  int need_to_refresh = 0;
  CURL *curl;
  CURLcode res = CURLE_OK;
  int break_pos = 0;
  
  struct MemoryStruct chunk;

  chunk.memory = (char*) malloc(1);  //crecerá según sea necesario con el realloc
  chunk.size = 0;
 
  curl = curl_easy_init();
  if(curl) {
    /* Set username and password */ 
    curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pwd);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 40);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	char tstr[100];
	if(atoi(port)==995)	strcpy(tstr,"pop3s://");
	else strcpy(tstr,"pop3://");
	strcat(tstr,server);
	if(strlen(port)>0 && atoi(port)!=0) 
	{
		strcat(tstr,":");
		strcat(tstr,port);
	}
    curl_easy_setopt(curl, CURLOPT_URL, tstr);
 
    /* Perform the list */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
	{
            stBar->push(curl_easy_strerror(res));
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
	}
	else{
        int *numbers;
		int n = extract_numbers(chunk,numbers);
		if(n>0 && n%2==0)
		{
			if(mCount==(n>>1))
			{
				int i;
				for(i = 0;i<mCount;i++) if(messages[i].m_size!=numbers[(i<<1)+1]) break;
				if(i<mCount) {need_to_refresh = 1;break_pos = i;}
			}
			else {need_to_refresh = 1;break_pos = mCount;}
			
			if(need_to_refresh)
			{
				int *oldspam = NULL;
				if(messages!=NULL) 
				{
					if(break_pos>0) 
					{
						oldspam = new int[break_pos<<1];
						for(int j = 0;j<break_pos;j++)
						{
							oldspam[j<<1] = messages[j].spam;
							oldspam[(j<<1)+1] = messages[j].deleteIt;
						}
					}
					delete[] messages;
				}
				oldCount = mCount;
				mCount = n>>1;
				messages = new MailMessage[mCount];
				
				for(int i = 0;i<n;i+=2) 
				{
					messages[i>>1].mbIndex = numbers[i];
					messages[i>>1].m_size = numbers[i+1];
				}
				if(break_pos>0) 
				{
					for(int j = 0;j<break_pos;j++)
					{
						messages[j].spam = oldspam[j<<1];
						messages[j].deleteIt = oldspam[(j<<1)+1];
					}
				}
				if(oldspam!=NULL) delete(oldspam);
			}
			delete(numbers);
		}
		else if(n==0) 
		{
			stBar->push("No messages in the mail box.");
			if(mCount>0)
			{
				mCount = 0;
				if(messages!=NULL) 
				{
					delete[] messages;
					messages = NULL;
				}
			}
		}
		else stBar->push("Weird error, count of numbers is odd.");
    }
	while (gtk_events_pending ()) gtk_main_iteration ();
	int new_valid = 0;
	if(need_to_refresh && mCount>0)
	{
	  for(int i = 0;i<mCount;i++)
	  {
		int oldsize = chunk.size;
		sprintf (tstr,"Retrieving message #%i",messages[i].mbIndex);
		stBar->push(tstr);
		while (gtk_events_pending ()) gtk_main_iteration ();
		
		sprintf (tstr,"TOP %i 12",messages[i].mbIndex);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, tstr);
	 
		res = curl_easy_perform(curl);
	 
		if(res != CURLE_OK)
		{
		  fprintf(stderr, "curl_easy_perform() failed: %s\n",
				  curl_easy_strerror(res));
		}
		else{
			int tsize = chunk.size + 1 - oldsize;
			//printf("%i %i %i\n",i,tsize,oldsize);
			messages[i].b_size = tsize;
			messages[i].buf = new char[tsize];
			memcpy(messages[i].buf,chunk.memory+oldsize,tsize-1);
			messages[i].buf[tsize-1] = 0;
			if(check_presense(messages[i].buf,whitelist)==false)
			{
			Glib::ustring tus = messages[i].Subject();
			int j = 0;
			for(j = 0;j<tus.length();j++)
				if(tus[j]>= 0x4E00 && tus[j]<= 0x2FA1F) break;
			if(j<tus.length()) 
			{
				messages[i].spam = true;
				messages[i].deleteIt = true;
			}
			else if(strstr(messages[i].buf,"GB2312") || strstr(messages[i].buf,"gb2312"))
			{
				messages[i].spam = true;
				messages[i].deleteIt = true;
			}
			else if(check_presense(messages[i].buf,aliases)==false)
			{
				messages[i].spam = true;
				messages[i].deleteIt = true;
			}
			else if(check_presense(messages[i].buf,blacklist)==true)
			{
				messages[i].spam = true;
				messages[i].deleteIt = true;
			}
			else if(i>break_pos)
			{
				char *buffer = messages[i].buf;
				int *positions = find_all_ips(buffer);
				if(positions!=NULL)
				{
					char t[50];
					int j = 0;
					while(positions[j]!=-1)
					{
						strncpy(t,buffer+positions[j],positions[j+1]);
						t[positions[j+1]] = 0;
						j+=2;
						if(strcmp(t,"127.0.0.1")!=0)
						{
							reverse_ip_string(t);
							strcat(t,".bl.spamcop.net");
							//printf("%s\n",t);
							struct hostent *lh = gethostbyname(t);
							if (lh) //puts(lh->h_name);
							{
								messages[i].spam = true;
								messages[i].deleteIt = true;
							}
							//else puts("not found");
						}
					}
					delete(positions);
				}
				//else puts("null");
			}
			}
			if(i>=oldCount && !messages[i].spam) new_valid = 1;
		}
	  }
	  sprintf (tstr,"%i messages fetched",mCount);
	  stBar->push(tstr);
	}
	else if(mCount>0) stBar->push("No new messages.");
	else need_to_refresh = 2;
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "QUIT");
	res = curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);
	if(new_valid == 0) need_to_refresh = 2;
  }
  if(chunk.memory!=NULL) free(chunk.memory);
  while (gtk_events_pending ()) gtk_main_iteration ();
  //stBar->remove_all_messages();
  return(need_to_refresh);
}


std::vector<Glib::ustring> explode(char* s, char delim)
{
    std::vector<Glib::ustring> result;
	int i = 0,last = 0;
	while(s[i]!=0)
    {
		if(s[i]==delim)
		{
			Glib::ustring tstr(s+last,i-last);
			last = i+1;
			result.push_back(tstr);
		}
		i++;
    }

    return result;
}

MailMessage::MailMessage()
{
	buf = NULL;
	b_size = 0;
	spam = false;
	deleteIt = false;
}
MailMessage::~MailMessage()
{
	if(buf!=NULL) delete(buf);
}

Glib::ustring MailMessage::Whole()
{
	Glib::ustring result = "";
	if(b_size>0 && buf!=NULL) result = buf;
	return(result);
}

// very questionable code I found in the net
int hexval(int c)
{
  if ('0' <= c && c <= '9') return c - '0';
  if('A' <= c && c <= 'F') return 10 + c - 'A';
  else return('0');
}
char *decode_quoted_printable(const char *body)
{
	char *t = new char[strlen(body)+10];
	int k = 0;
    char *end = (char*) body+strlen(body);
  while (*body && body<end) {
    if (*body != '=')
    {
        char tt = *body++;
        if(tt == '_') tt = ' ';
        t[k++] = tt;
    }
    else if (*(body+1) == '\r' && *(body+2) == '\n') body += 3;
    else if (*(body+1) == '\n') body += 2;
    else if (!strchr("0123456789ABCDEF", *(body+1))) t[k++] = *body++;
    else if (!strchr("0123456789ABCDEF", *(body+2))) t[k++] = *body++;
    else {
        t[k++] = 16 * hexval(*(body+1)) + hexval(*(body+2));
        body += 3;
    }
  }
  t[k] = 0;
  return(t);
}
void decode_header(char* &src)
{
	int do_check;
	do{
	do_check = 0;
	int len = strlen(src);
	char *t = new char[len+10];
	for(int i = 0;i<=len;i++) t[i] = toupper(src[i]);
	int clen = 7;
	char *pos1 = strstr(t,"=?UTF-8");
	//if(pos1==NULL) { pos1 = strstr(t,"=?GB2312");clen=8;}
	if(pos1!=NULL)
	{
        char *pos2;
        if(strncmp(t+clen+1,"?B?",3)==0 || strncmp(t+clen+1,"?Q?",3)==0) pos2 = strstr(t+clen+4,"?=");
        else pos2 = strstr(t+clen+1,"?=");
		if(pos2!=NULL)
		{
			do_check = 1;
			len = pos2-pos1-clen;
			int i = pos1+clen-t;
			int j;
			for(j = 0;j<len;j++) t[j] = src[i++];
			t[j] = 0;
            if(strncmp(t,"?B?",3)==0 || strncmp(t,"?b?",3)==0)
			{
				gsize len1 = 0;
				guchar *tt = g_base64_decode (t+3,&len1);
				if(len1>0)
				{
					for(i = 0;i<len1;i++) t[i] = tt[i];
					t[i] = 0;
				}
				if(tt!=NULL) g_free(tt);
				strcpy(t+strlen(t),&src[pos2-t+2]);
				strcpy(&src[pos1-t],t);
                strcpy(src,t);
			}
			else if(strncmp(t,"?Q?",3)==0 || strncmp(t,"?q?",3)==0)
			{
				char *tt = decode_quoted_printable(t+3);
				for(i = 0;tt[i]!=0;i++) t[i] = tt[i];
				t[i] = 0;
				delete(tt);
				strcpy(t+strlen(t),&src[pos2-t+2]);
				strcpy(&src[pos1-t],t);
                strcpy(src,t);
			}
			else do_check = 0;
		}
	}
	delete(t);
	}while(do_check);
}
Glib::ustring MailMessage::Subject()
{
	Glib::ustring tstr = "Subject:";
	if(b_size==0 || buf==NULL) return(tstr);
	char *p = strstr(buf,"\nSubject:");
	if(p) 
	{
		int i = p+9-buf;
		p = p+9;
		int pos = i;
		while(buf[i]!=0)
		{
			if(buf[i]==10 && !isspace((int)buf[i+1])) break;
			i++;
		}
		if(buf[i-1]==13) i--;
		i = i-pos;
		if(i>0 && i<b_size)
		{
			char *t = new char[i+1];
			int k = 0;
			for(int j = 0;j<i;j++) 
			{
				//if(p[j]<' ') t[j] = ' ';
				//else t[j] = p[j];
				if(p[j]>13) t[k++] = p[j];
			}
			t[i] = 0;
			decode_header(t);
			tstr = t;
			delete(t);
		}
	}
	return(tstr);
}
Glib::ustring MailMessage::Header()
{
	Glib::ustring tstr = "";
	if(b_size==0 || buf==NULL) return(tstr);
	int i = 0,last = 0;
	while(buf[i]!=0)
    {
		if(buf[i]==10 && buf[i+1]==13) break;
		i++;
    }
	if(buf[i]!=0) tstr=Glib::ustring(buf,i);
	return tstr;
}
Glib::ustring MailMessage::From()
{
	Glib::ustring tstr = "From:";
	if(b_size==0 || buf==NULL) return(tstr);
	char *p = strstr(buf,"\nFrom:");
	if(p) 
	{
		int i = p+6-buf;
		p = p+6;
		int pos = i;
		while(buf[i]!=0)
		{
			if(buf[i]==10) break;
			i++;
		}
		if(buf[i-1]==13) i--;
		i = i-pos;
		if(i>0 && i<b_size)
		{
			char *t = new char[i+1];
			for(int j = 0;j<i;j++) 
			{
				if(p[j]<' ') t[j] = ' ';
				else t[j] = p[j];
			}
			t[i] = 0;
			decode_header(t);
			tstr = t;
			delete(t);
		}
	}
	char ts[100];
	sprintf(ts,"%iKb",(int)(m_size/1024));
	tstr = ts+tstr;
	return tstr;
}
char *strip_tags(const char *str)
{
	char *buf = new char[strlen(str)+1];
	strcpy(buf,str);
	char *ts1 = strstr(buf,"<style");
	if(ts1!=NULL)
	{
		char *ts2 = strstr(ts1,"</style>");
		if(ts2!=NULL)
		{
			strcpy(ts1,ts2+strlen("</style>"));
		}
		else ts1[0] = 0;
	}
	int idx = 0;
	int opened = 0; // false
	for(int i=0; buf[i]!=0; i++)
	{
		if(buf[i]=='<') {
			opened = 1; // true
		} else if (buf[i] == '>') {
			opened = 0; // false
		} else if (!opened) {
			buf[idx++] = buf[i];
		}
	}
	buf[idx] = '\0';
	return(buf);
}
Glib::ustring MailMessage::Body()
{
	Glib::ustring tstr = "";
	if(b_size==0 || buf==NULL) return(tstr);
	int i = 0;
	while(buf[i]!=0)
    {
		if(buf[i]==10 && buf[i+1]==13) break;
		i++;
    }
	if(buf[i]!=0)
	{
		char *t = strip_tags(buf+i+1);
		if(char *t1=strstr(t,"ncoding: quoted-printable"))
		{
			t1 = decode_quoted_printable(t1+strlen("ncoding: quoted-printable"));
			delete(t);
			t = t1;
		}
		if(!strstr(t,"charset=\"utf-8\""))
		{
			try{
				tstr = Glib::convert_with_fallback(t, "UTF-8","CP1251");
			}
			catch(Glib::Error&)
			{
				tstr = t;//"Can't convert string";
			}
		}
		else tstr = t;
		delete(t);
	}
	else tstr = "Strange body format or no body";
	//tstr = buf;
 	return tstr;
}
