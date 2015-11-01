/* Copyright 2015 Óscar Pereira */

/* Licensed under the Apache License, Version 2.0 (the "License"); */
/* you may not use this file except in compliance with the License. */
/* You may obtain a copy of the License at */

/*     http://www.apache.org/licenses/LICENSE-2.0 */

/* Unless required by applicable law or agreed to in writing, software */
/* distributed under the License is distributed on an "AS IS" BASIS, */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/* See the License for the specific language governing permissions and */
/* limitations under the License. */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <libnotify/notify.h>
#include <sys/inotify.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define LOG_LEN     1024  
#define DATE_LEN    20
#define RCPT_LEN    60

volatile sig_atomic_t done = 0;

static void setup(int, char **, char ** );
static void usage(char *);
static void send_notify(char* msg1, char* msg2);

// Signal handler
void term(int signum)
{
	done = 1;
}

int main(int argc, char **argv)
{
	int length, idx = 0;
  int fd;
  int wd;
	FILE* ld;
  char buffer[BUF_LEN];
  char logmsg[LOG_LEN];
	char date[DATE_LEN];
	char rcpt[RCPT_LEN];
	char *p = NULL;
  char *msmtplog = "./msmtp.log";

  setup(argc, argv, &msmtplog);

	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
 
  fd = inotify_init();

  if ( fd < 0 ) {
    perror( "inotify_init" );
  }

  wd = inotify_add_watch( fd, msmtplog, IN_CLOSE_WRITE);

	while(!done) {
		// wait for something to be written in MSMTP's log file...
		if ( (length = read(fd, buffer, BUF_LEN)) < 0) {
			if ( length == -1 && errno == EINTR )
				break;
			else {
				perror( "read" );
			}  
		}

		// process inotify events
		idx = 0;
		int r;
		while ( idx < length ) {
			struct inotify_event *event = ( struct inotify_event * ) &buffer[ idx ];
			if ( event->mask & IN_CLOSE_WRITE ) {
				if ((ld = fopen(msmtplog, "r")) == NULL){
					err(1, "fopen(%s)", msmtplog);
					break;
				}

				// read last line (LOG_LEN greater than line length).
				fseek(ld, -LOG_LEN, SEEK_END);
				while(!feof(ld))
        {
					memset(logmsg, '\0', sizeof(logmsg));
					if ((r=fscanf(ld, "%[^\n]\n", logmsg)) == EOF) {
						if ( r == -1 && errno == EINTR )
							done = 1;
						else {
							perror( "read" );
						}
					}
        }
				fclose(ld);
				// logmsg now contains last line

				memset(date, '\0', sizeof(date));
				memset(rcpt, '\0', sizeof(rcpt));

				p = strtok(logmsg, " ");

				// get date (it's at start of line). Format: Oct 31 17:23:18
				strncat(date, p, 3);
				strncat(date, " ", 1);
				p = strtok(NULL, " ");
				strncat(date, p, 2);
				strncat(date, " ", 1);
				p = strtok(NULL, " ");
				strncat(date, p, 8);
				// date is now in char[] date

				// get exit status and mail TO: 
				int success = 0;
				while (1) {
					p = strtok(NULL, " ");
					if(!p)
						break;
					if (!strncmp(p, "recipients=", 11) && strlen(p) > 11){
						strncpy(rcpt, p+11, RCPT_LEN - 6); // copy what is after the '=' sign

						// if there is more than 1 rcpt, then they are listed, comma separated,
						// after the '='. The while loop detects this and deletes all but the 
						// **first** rcpt, replacing the remaining with ' ... '.
						char* c = rcpt;
						while(*c) {
							if(*c == ',') {
								*(c+1)=' ';
								*(c+2)='.';
								*(c+3)='.';
								*(c+4)='.';
								*(c+5)=' ';
								*(c+6)='\0';
								break;
							} else
								c++;
						} // end rcpt parsing
					}
					if (!strncmp(p, "exitcode=EX_OK", 14))
						success = 1;
					if (!strncmp(p, "recipients=", 11)){
						strncpy(rcpt, p+11, strlen(p)-11);
					}
				}
				// exit status is in int success and rcpt in char[] rcpt.

				char notify_cmd[256];
				memset(notify_cmd, '\0', sizeof(notify_cmd));

				// prepare and send notification
				if(success) {
					sprintf(notify_cmd, "Sent TO: %s, AT %s.\n", rcpt, date);
					send_notify("Mail Sent *SUCCESSFULLY*", notify_cmd);
				}
				else {
					sprintf(notify_cmd, "(Should have been...) sent TO: \
							%s, AT %s.\n", rcpt, date);
					send_notify("Mail Sent *FAILED*!", notify_cmd);
				}
				// notification sent

			}
			idx += EVENT_SIZE + event->len; // go to next inotify event
		} // end while notify events
	} // end while(!done)

	( void ) inotify_rm_watch( fd, wd );
	( void ) close( fd );

  return 0;
}

static void setup(int argc, char **argv, char **msmtplog)
{
  int c;

  while ((c = getopt(argc, argv, "l:h")) != -1)
    switch (c) {
    case 'l':
      *msmtplog = optarg;
      break;
    case 'h':
    case '?':
    default:
      usage(argv[0]);
    }
  if (optind < argc)
    usage(argv[0]);
  if (argc == 1)
    usage(argv[0]);
}

static void send_notify(char* msg1, char* msg2) {
	notify_init ("MSMTP notify");
	NotifyNotification * Hello = notify_notification_new (msg1, msg2, "dialog-information");
	notify_notification_set_timeout(Hello, NOTIFY_EXPIRES_NEVER);
	notify_notification_show (Hello, NULL);
	g_object_unref(G_OBJECT(Hello));
	notify_uninit();
}

static void usage(char *progname)
{
  fprintf(stderr, "usage:    %s  ", progname);
  fprintf(stderr, "[ -l msmtplog ]\n");
  fprintf(stderr, "default: -l ./msmtp.log\n");
  exit(2);
}
