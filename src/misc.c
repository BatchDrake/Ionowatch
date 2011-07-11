/*
    This file is part of Ionowatch.

    (c) 2011 Gonzalo J. Carracedo <BatchDrake@gmail.com>
    
    Ionowatch is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ionowatch is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <libgen.h>

#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>

#include <ionowatch.h>


PTR_LIST (struct station_info, stations);

struct station_info *
station_info_new (const char *line)
{
  struct station_info *new;
  arg_list_t *list;
  char *comma;
  
  float lat, lon;
  int i;
  
  list = csv_split_line (line);
  
  if (list->al_argc < STATION_FILE_FIELDS_MINIMUM)
  {
    ERROR ("malformed line (%d fields are not enough)\n", list->al_argc);
    free_al (list);
    
    return NULL;
  }
 
  if (station_lookup (list->al_argv[STATION_FILE_LINE_NAME]) != NULL)
  {
    free_al (list);
    return NULL;
  }
  
  comma = strchr (list->al_argv[STATION_FILE_LINE_LATITUDE], ',');
  
  if (comma != NULL)
    *comma = '.';
    
  comma = strchr (list->al_argv[STATION_FILE_LINE_LONGITUDE], ',');
  
  if (comma != NULL)
    *comma = '.';
    
    
  if (!sscanf (list->al_argv[STATION_FILE_LINE_LATITUDE], "%f", &lat) ||
      !sscanf (list->al_argv[STATION_FILE_LINE_LONGITUDE], "%f", &lon))
  {
    ERROR ("station coordinates are wrong\n");
    free_al (list);
    
    return NULL;
  }
               
  new = xmalloc (sizeof (struct station_info));
  
  new->args = list;
  new->name_long = list->al_argv[STATION_FILE_LINE_LONG_NAME];
  new->name = list->al_argv[STATION_FILE_LINE_NAME];
  
  for (i = STATION_FILE_LINE_COUNTRIES; i < list->al_argc; i++)
    if (strlen (list->al_argv[i]))
      break;
      
  if (i == list->al_argc)
    i--;
    
  new->country = list->al_argv[i];
  new->lat = lat;
  new->lon = lon;
  new->realtime = strcmp (list->al_argv[STATION_FILE_LINE_REALTIME], "Y") == 0;
  
  return new;
}

struct station_info *
station_info_pick_rand (void)
{
  int n;
  
  for (;;)
  {
    n = (stations_count - 1) * ((float) rand () / (float) RAND_MAX);
    
    if (stations_list[n] != NULL)
      return stations_list[n];
  }
}

void
station_info_destroy (struct station_info *info)
{
  free_al (info->args);
  free (info);
}

void
station_debug (struct station_info *info)
{
  NOTICE ("%s: %gE, %gN %s (%s)\n", 
    info->name, info->lon, info->lat, info->name_long, info->country);
}

int
parse_station_file (const char *path)
{
  int i, n;
  
  FILE *fp;
  char line[RECOMMENDED_LINE_SIZE];
  struct station_info *info;
  
  if ((fp = fopen (path, "rb")) == NULL)
  {
    ERROR ("couldn't open %s: %s\n", path, strerror (errno));
    return -1;
  }
  
  i = 0;
  
  while (!feof (fp))
  {
    fgets (line, RECOMMENDED_LINE_SIZE, fp);
    
    line[RECOMMENDED_LINE_SIZE - 1] = '\0'; /* Inconsistent implementation */
    
    n = strlen (line) - 1;
    
    while (n >= 0 && isspace (line[n]))
      line[n--] = 0;
      
    if (n < 0)
      continue;
      
    info = station_info_new (line);
    
    if (info != NULL)
    {
      PTR_LIST_APPEND (stations, info);
      i++;
    }
  }
  
  fclose (fp);
  
  return i;
}

struct station_info *
station_lookup (const char *code)
{
  int i;
  
  for (i = 0; i < stations_count; i++)
    if (strcasecmp (stations_list[i]->name, code) == 0)
      return stations_list[i];
      
  return NULL;
}

int 
sck_connect (const char *host, int port)
{
  struct sockaddr_in sockaddr_server;
  int csfd;
  struct hostent *ent;
  struct in_addr *addr;
  char *ip;

  ent = gethostbyname (host);
  
  if (ent == NULL)
    return -1;
    
  addr =  (struct in_addr *) ent->h_addr;
  ip = inet_ntoa ((struct in_addr) (*addr));

  
  sockaddr_server.sin_family = AF_INET;
  inet_aton (ip, &sockaddr_server.sin_addr);
  sockaddr_server.sin_port = htons (port);

  fflush (stdout);
  csfd = socket (PF_INET, SOCK_STREAM, 0);
  
  if (csfd == -1)
    return -1;
    
  if (connect (csfd,  (struct sockaddr*) &sockaddr_server, sizeof (struct sockaddr_in)) == -1)
  {
    close (csfd);
    return -1;
  }

  return csfd;
}

static int 
dgets (int fd, char *buf, int max)
{
  int i;
  for (i = 0; i < max; i++)
  {
    if (i >= (max-1))
    {
      buf [i] = '\0';
      break;
    }
    if (read (fd, &buf[i], 1) < 1)
      return -1;
      
    if (buf[i] == '\r')
    {
      i--;
      continue;
    }
    
    if (buf[i] == '\n')
    {
      buf[i] = '\0';
      break;
    }
  }
  
  return i;
}

int 
http_download (const char *addr_got, FILE *fp)
{
  char *server;
  char *path;
  void *chunkbuf;
	char *address;
	
  int i, p, j, n;
  int ischunk, size;
  unsigned int port = 80;
	
  char blockbuf[4096];
  int sfd;
	
	address = xstrdup (addr_got);
		
  if (strncasecmp (address, "http://", 7))
  {
    ERROR ("invalid location «%s». It must start with http://\n", address);
    free (address);
    return -1;
  }
	
  for (i = 7; i < strlen (address) + 1; i++)
  {
    if (address[i] == '\0' || address[i] == '/')
    {
      p = i - 7;
      if (p == 0)
      {
        ERROR ("no server specified\n");
        free (address);
        return -1;
      }
			
      server = xmalloc (p + 1);
			
      strncpy (server, &address[7], p);
      server[p] = '\0';
      break;
    }
  }
	
  for (j = 0; j < strlen (server); j++)
    if (server[j] == ':')
  {
    server[j] = '\0';
    if (sscanf (&server[j + 1], "%i", &port) == 0)
    {
      ERROR ("wrong port, using standard (80)\n");
      port = 80;
    }
    else if (port < 1 || port > 65535)
    {
      ERROR ("wrong port, using standard (80)\n");
      port = 80;
    }
  }
		

  if (address[i] == '\0')
    path = "";
  else
    path = &address[i + 1];
	
  sfd = sck_connect (server, port);
	
  if (sfd == -1)
  {
    ERROR ("error while connecting to %s:%d.\n", server, port);
    free (server);
    free (address);
    return -1;
  }
	
  dprintf (sfd, "GET /%s HTTP/1.1\r\n", path);
  dprintf (sfd, "Host: %s\r\n", server);
  dprintf (sfd, "Connection: close\r\n");
  dprintf (sfd, "Cache-Control: max-age=0\r\n");
  dprintf (sfd, "User-Agent: Ionowatch/0.1 (BatchDrake@gmail.com)\r\n");
  dprintf (sfd, "Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n");
  dprintf (sfd, "Accept-Encoding: deflate\r\n");
  dprintf (sfd, "Accept-Language: es-ES,es;q=0.8\r\n");
  dprintf (sfd, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n");
	
  /* Enviadas las cadenas de petición... vamos a lo otro */
  if (dgets (sfd, blockbuf, 4096) == -1)
  {
    NOTICE ("error while getting response form server\r\n");
    free (server);
    free (address);
    return -1;
  }
	
  if (strncmp (&blockbuf[9], "200", 3))
  {
    DEBUG ("http %c%c%c on %s\n", blockbuf[9], blockbuf[10], blockbuf[11], addr_got); 
    free (server);
    close (sfd);
    free (address);
    return -1;
  }
	
  ischunk = 0;
  size = 0;
	
  while (dgets (sfd, blockbuf, 4096) != -1)
  {
    if (blockbuf[0] == '\0')
      break;
		
    if (strncasecmp (blockbuf, "Transfer-Encoding: chunked", 26) == 0)
      ischunk = 1;
  }

  if (ischunk)
  {
    for (;;)
    {
      if (dgets (sfd, blockbuf, 4096) == -1)
      {
        ERROR ("connection with temote host %s closed unexpectedly\n", server);
        free (server);
        close (sfd);
        free (address);
        return -1;
      }
			
      if (!strlen (blockbuf))
        continue;
			
      if (!sscanf (blockbuf, "%x", &size))
      {
        ERROR ("error reading chunk size from %s\n", server);
        free (server);
        close (sfd);
				free (address);
        return -1;
      }
		
      /* El último chunk se omite */
			
      if (!size)
        break;
			
			
      chunkbuf = xmalloc (size);
			
			
      for (i = 0; i < size; i++)
        if (recv (sfd, chunkbuf + i, 1, 0) < 1)
      {
        ERROR ("connection with %s closed unexpectedly\n", server);
        goto break_parse;
      }
			
      fwrite (chunkbuf, size, 1, fp);
      free (chunkbuf);
    }
		
  }
  else
    while ( (n = recv (sfd, blockbuf, 4096, 0)) >= 1)
      fwrite (blockbuf, n, 1, fp);

break_parse:

  free (server);
  close (sfd);
  free (address);
  return 0;
}

struct strlist *
parse_apache_index (const char *url)
{
  struct strlist *list;
  char indexline[RECOMMENDED_LINE_SIZE];
  FILE *fp;
  char *hreflink;
  char *hrefstop;
  int p = 0;
  
  if ((fp = tmpfile ()) == NULL)
  {
    ERROR ("coudln't create temporary file: %s\n", strerror (errno));
    return NULL;
  }
  
  list = strlist_new ();
  
  (void) http_download (url, fp);
  
  fseek (fp, 0, SEEK_SET);
  
  while (!feof (fp))
  {
    if (fgets (indexline, RECOMMENDED_LINE_SIZE, fp) == NULL)
      break;
      
    indexline[RECOMMENDED_LINE_SIZE - 1] = '\0';
    
    /* WARNING WARNING WARNING */
    /* Code below is extremely LIKELY to be changed in the future. Not by
       my own decision, but by the decision of SPIDR sysadmins. This code
       relies in the assumption that the index format won't change too
       much in the future, assumption that is clearly arbitrary.
       
       This must have a better implementation, but by the time this
       project is done (I expect... the end of june of 2011) the index
       format won't change that much. Probably. */
       
#define APACHE_FILE_ENTRY_STRING "<tr><td valign=\"top\"><img src=\"/icons/"
#define APACHE_FILE_LINK         "<a href=\""
#define APACHE_FILE_LINK_STOP    "\">"

    if (strncmp (indexline, 
                 APACHE_FILE_ENTRY_STRING, 
                 sizeof (APACHE_FILE_ENTRY_STRING) - 1) == 0)
    {
      hreflink = strstr (indexline, APACHE_FILE_LINK);
      
      if (hreflink != NULL)
      {
        hreflink += sizeof (APACHE_FILE_LINK) - 1;
        hrefstop = strstr (hreflink, APACHE_FILE_LINK_STOP);
        
        if (hrefstop != NULL);
        {
          do
            *hrefstop-- = '\0';
          while (*hrefstop == '/');
          
          if (p++)
            strlist_append_string (list, basename (hreflink));
        }
      }
    }
     
  }
  
  fclose (fp);

#undef APACHE_FILE_ENTRY_STRING
#undef APACHE_FILE_LINK
#undef APACHE_FILE_LINK_STOP

  return list;
}

int
load_stations (void)
{
  char *path;
  int howmany;
  
  if ((path = locate_config_file ("stations.csv", 
       CONFIG_GLOBAL | CONFIG_READ)) == NULL)
  {
    ERROR ("couldn't open stations.csv: %s\n", strerror (errno));
    return -1;
  }
  
  howmany = parse_station_file (path);
  
  if (howmany < 1)
    ERROR ("couldn't load any station from %s\n", path);
  else
    NOTICE ("%d stations loaded\n", howmany);
    
  free (path);
  
  return howmany;
}

/* TODO: wrap this into a higher-level function */
int
ionowatch_config_init (void)
{
  srand (time (NULL));
  
  setenv ("TZ", "UTC", 1);
  
  if (get_ionowatch_config_dir () == NULL)
    return -1;
    
  if (get_ionowatch_cache_dir () == NULL)
    return -1;
  
  /* TODO: load stations from NOAA website */
  if (load_stations () == -1)
    return -1;
  
  if (init_sunspot_cache () == -1)
    return -1;
  
  if (load_all_plugins () == -1) /* What could happen? */
    return -1;
    
  /* Reload cache data --> urls and a lot more*/
  
  /* Be careful: if path doesn't have the "/" at its end, server will
     bitch us with a 301 Moved permanently */
     
  register_ionogram_inventory 
    ("Master Ionosonde Data Set", "http://ngdc.noaa.gov/ionosonde/MIDS/data/");
  
  ionogram_filetype_register 
    ("SAO", "Standard scaled data archive", parse_sao_file, sao_build_url);
    
  ionogram_filetype_register 
    ("MMM", "Modified maximum method ionogram", ionogram_from_mmm, blockfile_build_url);
    
  ionogram_filetype_register 
    ("SBF", "Single block format ionogram", ionogram_from_sbf, blockfile_build_url);
    
  ionogram_filetype_register 
    ("RSF", "Routine scientific format ionogram", ionogram_from_rsf, blockfile_build_url);
    
  return 0;
}

