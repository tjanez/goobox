/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include <glib.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <gconf/gconf-client.h>
#include "file-utils.h"


#define BUF_SIZE 4096
#define MAX_PATTERNS 128


gboolean
path_exists (const gchar *path)
{
        GnomeVFSFileInfo *info;
        GnomeVFSResult result;
        gboolean exists;
        gchar *escaped;

        if (! path || ! *path) return FALSE;

        info = gnome_vfs_file_info_new ();
        escaped = gnome_vfs_escape_path_string (path);
        result = gnome_vfs_get_file_info (escaped,
                                          info,
                                          (GNOME_VFS_FILE_INFO_DEFAULT
                                           | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));

        exists = (result == GNOME_VFS_OK);

        g_free (escaped);
        gnome_vfs_file_info_unref (info);

        return exists;
}


gboolean
uri_exists (const char  *uri)
{
        GnomeVFSFileInfo *info;
        GnomeVFSResult    result;
        gboolean          exists;

        if (uri == NULL) 
                return FALSE;

        info = gnome_vfs_file_info_new ();
        result = gnome_vfs_get_file_info (uri,
                                          info,
                                          GNOME_VFS_FILE_INFO_DEFAULT);

        exists = (result == GNOME_VFS_OK);

        gnome_vfs_file_info_unref (info);

        return exists;  
}


gboolean
uri_is_file (const char *uri)
{
        GnomeVFSFileInfo *info;
        GnomeVFSResult    result;
        gboolean          is_file;

        if (! uri || ! *uri)
                return FALSE;

        info = gnome_vfs_file_info_new ();
        result = gnome_vfs_get_file_info (uri,
                                          info,
                                          (GNOME_VFS_FILE_INFO_DEFAULT
                                           | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
        is_file = FALSE;
        if (result == GNOME_VFS_OK)
                is_file = (info->type == GNOME_VFS_FILE_TYPE_REGULAR);

        gnome_vfs_file_info_unref (info);

        return is_file;
}


gboolean
uri_is_dir (const char *uri)
{
        GnomeVFSFileInfo *info;
        GnomeVFSResult    result;
        gboolean          is_dir;

        if (! uri || ! *uri)
                return FALSE;

        info = gnome_vfs_file_info_new ();
        result = gnome_vfs_get_file_info (uri,
                                          info,
                                          (GNOME_VFS_FILE_INFO_DEFAULT
                                           | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
        is_dir = FALSE;
        if (result == GNOME_VFS_OK)
                is_dir = (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);

        gnome_vfs_file_info_unref (info);

        return is_dir;
}


gboolean
dir_is_empty (const gchar *path)
{
	DIR *dp;
	int n;

	if (strcmp (path, "/") == 0)
		return FALSE;

	dp = opendir (path);
	n = 0;
	while (readdir (dp) != NULL) {
		n++;
		if (n > 2) {
			closedir (dp);
			return FALSE;
		}
	}
	closedir (dp);

	return TRUE;
}


/* Check whether the path_src is contained in path_dest */
gboolean
path_in_path (const char  *path_src,
	      const char  *path_dest)
{
	int path_src_l, path_dest_l;

	if ((path_src == NULL) || (path_dest == NULL))
		return FALSE;
	
	path_src_l = strlen (path_src);
	path_dest_l = strlen (path_dest);
	
	return ((path_dest_l > path_src_l)
		&& (strncmp (path_src, path_dest, path_src_l) == 0)
		&& ((path_src_l == 1) || (path_dest[path_src_l] == '/')));
}


GnomeVFSFileSize 
get_file_size (const gchar *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	GnomeVFSFileSize size;
	gchar *escaped;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = gnome_vfs_escape_host_and_path_string (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info,
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS)); 
	size = 0;
	if (result == GNOME_VFS_OK)
		size = info->size;

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return size;
}


time_t 
get_file_mtime (const gchar *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	gchar *escaped;
	time_t mtime;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = gnome_vfs_escape_host_and_path_string (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	mtime = 0;
	if (result == GNOME_VFS_OK)
		mtime = info->mtime;

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return mtime;
}


time_t 
get_file_ctime (const gchar *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	gchar *escaped;
	time_t ctime;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = gnome_vfs_escape_host_and_path_string (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT 
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	ctime = 0;
	if (result == GNOME_VFS_OK)
		ctime = info->ctime;

	g_free (escaped);
	gnome_vfs_file_info_unref (info);

	return ctime;
}


gboolean 
file_copy (const char *source_uri, 
	   const char *dest_uri)
{
        GnomeVFSURI    *real_source_uri;
        GnomeVFSURI    *real_dest_uri;
        GnomeVFSResult  result;
                
        real_source_uri = gnome_vfs_uri_new (source_uri);
        real_dest_uri = gnome_vfs_uri_new (dest_uri);
                
        result = gnome_vfs_xfer_uri (real_source_uri, 
        			     real_dest_uri,
                                     GNOME_VFS_XFER_RECURSIVE, 
                                     GNOME_VFS_XFER_ERROR_MODE_ABORT,
                                     GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
                                     NULL, NULL);
                
        gnome_vfs_uri_unref (real_source_uri);
        gnome_vfs_uri_unref (real_dest_uri);
                
        return result == GNOME_VFS_OK;
}


gboolean
file_move (const gchar *from, 
	   const gchar *to)
{
	if (file_copy (from, to) && ! unlink (from))
		return TRUE;

	return FALSE;
}


gboolean 
file_is_hidden (const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0') return FALSE;
	if ((name[1] == '.') && (name[2] == '\0')) return FALSE;

	return TRUE;
}


/* like g_basename but does not warns about NULL and does not
 * alloc a new string. */
G_CONST_RETURN gchar *
file_name_from_path (const gchar *file_name)
{
	register gssize base;
	register gssize last_char;

	if (file_name == NULL) 
		return NULL;

	if (file_name[0] == '\0')
		return "";

	last_char = strlen (file_name) - 1;

	if (file_name [last_char] == G_DIR_SEPARATOR)
		return "";

	base = last_char;
	while ((base >= 0) && (file_name [base] != G_DIR_SEPARATOR))
		base--;

	return file_name + base + 1;
}


gchar *
remove_level_from_path (const gchar *path)
{
	gchar *new_path;
	const gchar *ptr = path;
	gint p;

	if (! path) return NULL;

	p = strlen (path) - 1;
	if (p < 0) return NULL;

	while ((ptr[p] != '/') && (p > 0)) p--;
	if ((p == 0) && (ptr[p] == '/')) p++;
	new_path = g_strndup (path, (guint)p);

	return new_path;
}


gchar *
remove_extension_from_path (const gchar *path)
{
	gchar *new_path;
	const gchar *ptr = path;
	gint p;

	if (!path) return NULL;
	if (strlen (path) < 2) return g_strdup( path);

	p = strlen (path) - 1;
	while ((ptr[p] != '.') && (p > 0)) p--;
	if (p == 0) p = strlen (path) - 1;
	new_path = g_strndup (path, (guint) p);

	return new_path;
}


gchar * 
remove_ending_separator (const gchar *path)
{
	gint len, copy_len;

	if (path == NULL)
		return NULL;

	copy_len = len = strlen (path);
	if ((len > 1) && (path[len - 1] == '/')) 
		copy_len--;

	return g_strndup (path, copy_len);
}


GnomeVFSResult
ensure_dir_exists (const gchar *uri,
		   mode_t       mode)
{
	char *path;
	char *p;
	char *scheme;

	if (uri == NULL) 
		return GNOME_VFS_ERROR_BAD_PARAMETERS;
	
	if (uri_is_dir (uri))
		return GNOME_VFS_OK;

	path = g_strdup (uri);
	p = path;

	scheme = gnome_vfs_get_uri_scheme (path);
	if (scheme != NULL) {
		p += strlen (scheme);
		if (strncmp (p, "://", 3) == 0)
			p += 3;
	}
	
	while (*p != '\0') {
		gboolean end;

		p++;
		if ((*p != '/') && (*p != '\0')) 
			continue;

		end = TRUE;
		if (*p != '\0') {
			*p = '\0';
			end = FALSE;
		}
			
		if (! uri_is_dir (path)) {
			GnomeVFSResult result;
			result = gnome_vfs_make_directory (path, mode);
			if (result != GNOME_VFS_OK) {
				g_free (path);
				return result;
			}
		}

		if (! end) 
			*p = '/';
	}
	
	g_free (scheme);
	g_free (path);
	
	return GNOME_VFS_OK;
}


gboolean
file_extension_is (const char *filename, 
		   const char *ext)
{
	int filename_l, ext_l;
	
	filename_l = strlen (filename);
	ext_l = strlen (ext);
	
	if (filename_l < ext_l)
		return FALSE;
	return strcasecmp (filename + filename_l - ext_l, ext) == 0;
}


gboolean
is_mime_type (const char* type, const char* pattern) {
	return (strncasecmp (type, pattern, strlen (pattern)) == 0);
}


void
path_list_free (GList *path_list)
{
	if (path_list != NULL) {
		g_list_foreach (path_list, (GFunc) g_free, NULL);
		g_list_free (path_list);
	}
}


GList *
path_list_dup (GList *path_list)
{
	GList *new_list = NULL;
	GList *scan;

	for (scan = path_list; scan; scan = scan->next)
		new_list = g_list_prepend (new_list, g_strdup (scan->data));

	return g_list_reverse (new_list);
}


/* counts how many characters to escape in @str. */
static int
count_chars_to_escape (const char *str, 
		       const char *meta_chars)
{
	int         meta_chars_n = strlen (meta_chars);
	const char *s;
	int         n = 0;

	for (s = str; *s != 0; s++) {
		int i;
		for (i = 0; i < meta_chars_n; i++) 
			if (*s == meta_chars[i]) {
				n++;
				break;
			}
	}
	return n;
}


gboolean
strchrs (const char *str,
	 const char *chars)
{
	const char *c;
	for (c = chars; *c != '\0'; c++)
		if (strchr (str, *c) != NULL)
			return TRUE;
	return FALSE;
}


char*
escape_str_common (const char *str, 
		   const char *meta_chars,
		   const char  prefix,
		   const char  postfix)
{
	int         meta_chars_n = strlen (meta_chars);
	char       *escaped;
	int         i, new_l, extra_chars = 0;
	const char *s;
	char       *t;

	if (str == NULL) 
		return NULL;

	if (prefix)
		extra_chars++;
	if (postfix)
		extra_chars++;

	new_l = strlen (str) + (count_chars_to_escape (str, meta_chars) * extra_chars);
	escaped = g_malloc (new_l + 1);

	s = str;
	t = escaped;
	while (*s) {
		gboolean is_bad = FALSE;
		for (i = 0; (i < meta_chars_n) && !is_bad; i++)
			is_bad = (*s == meta_chars[i]);
		if (is_bad && prefix)
			*t++ = prefix;
		*t++ = *s++;
		if (is_bad && postfix)
			*t++ = postfix;
	}
	*t = 0;

	return escaped;
}


/* escape with backslash the string @str. */
char*
escape_str (const char *str, 
	    const char *meta_chars)
{
	return escape_str_common (str, meta_chars, '\\', 0);
}


/* escape with backslash the file name. */
char*
shell_escape (const char *filename)
{
	return escape_str (filename, "$\'`\"\\!?* ()[]<>&|@#:;");
}


char *
escape_uri (const char *uri)
{
        const char *start = NULL;
        const char *uri_no_method;
        char       *method;
        char       *epath, *euri;

        if (uri == NULL)
                return NULL;

        start = strstr (uri, "://");
        if (start != NULL) {
                uri_no_method = start + strlen ("://");
                method = g_strndup (uri, start - uri);
        } else {
                uri_no_method = uri;
                method = NULL;
        }

	epath = gnome_vfs_escape_host_and_path_string (uri_no_method);

        if (method != NULL) {
                euri = g_strdup_printf ("%s://%s", method, epath);
                g_free (epath);
        } else
                euri = epath;

        g_free (method);

        return euri;
}


static gchar *
get_terminal ()
{
	GConfClient *client;
	gchar *result;
	gchar *terminal = NULL;
	gchar *exec_flag = NULL;

	client = gconf_client_get_default ();
	terminal = gconf_client_get_string (client, "/desktop/gnome/applications/terminal/exec", NULL);
	g_object_unref (G_OBJECT (client));
	
	if (terminal) 
		exec_flag = gconf_client_get_string (client, "/desktop/gnome/applications/terminal/exec_arg", NULL);

	if (terminal == NULL) {
		char *check;

		check = g_find_program_in_path ("gnome-terminal");
		if (check != NULL) {
			terminal = check;
			/* Note that gnome-terminal takes -x and
			 * as -e in gnome-terminal is broken we use that. */
			exec_flag = g_strdup ("-x");
		} else {
			if (check == NULL)
				check = g_find_program_in_path ("nxterm");
			if (check == NULL)
				check = g_find_program_in_path ("color-xterm");
			if (check == NULL)
				check = g_find_program_in_path ("rxvt");
			if (check == NULL)
				check = g_find_program_in_path ("xterm");
			if (check == NULL)
				check = g_find_program_in_path ("dtterm");
			if (check == NULL) {
				g_warning (_("Cannot find a terminal, using "
					     "xterm, even if it may not work"));
				check = g_strdup ("xterm");
			}
			terminal = check;
			exec_flag = g_strdup ("-e");
		}
	}

	if (terminal == NULL)
		return NULL;

	result = g_strconcat (terminal, " ", exec_flag, NULL);
	return result;
}


gchar *
application_get_command (const GnomeVFSMimeApplication *app)
{
	char *command;
	const char *bad_chars = "$\'`\"\\!?*()[]&|@#:;"; /* similar to shell_escape but without ' ' */
	
	if (app->requires_terminal) {
		char *terminal;
		char *command_to_exec;

		terminal = get_terminal ();
		if (terminal == NULL)
			return NULL;

		command_to_exec = escape_str (app->command, bad_chars);
		command = g_strconcat (terminal,
				       " ",
				       command_to_exec,
				       NULL);
		g_free (terminal);
		g_free (command_to_exec);
	} else
		command = escape_str (app->command, bad_chars);

	return command;
}


static const char *
g_utf8_strstr (const char *haystack, const char *needle)
{
	const char *s;
	gsize       i;
	gsize       haystack_len = g_utf8_strlen (haystack, -1);
	gsize       needle_len = g_utf8_strlen (needle, -1);
	int         needle_size = strlen (needle);

	s = haystack;
	for (i = 0; i <= haystack_len - needle_len; i++) {
		if (strncmp (s, needle, needle_size) == 0)
			return s;
		s = g_utf8_next_char(s);
	}

	return NULL;
}


static char**
g_utf8_strsplit (const char *string,
		 const char *delimiter,
		 int         max_tokens)
{
	GSList      *string_list = NULL, *slist;
	char       **str_array;
	const char  *s;
	guint        n = 0;
	const char  *remainder;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != NULL, NULL);
	g_return_val_if_fail (delimiter[0] != '\0', NULL);
	
	if (max_tokens < 1)
		max_tokens = G_MAXINT;
	
	remainder = string;
	s = g_utf8_strstr (remainder, delimiter);
	if (s != NULL) {
		gsize delimiter_size = strlen (delimiter);
		
		while (--max_tokens && (s != NULL)) {
			gsize  size = s - remainder;
			char  *new_string;

			new_string = g_new (char, size + 1);
			strncpy (new_string, remainder, size);
			new_string[size] = 0;

			string_list = g_slist_prepend (string_list, new_string);
			n++;
			remainder = s + delimiter_size;
			s = g_utf8_strstr (remainder, delimiter);
		}
	}
	if (*string) {
		n++;
		string_list = g_slist_prepend (string_list, g_strdup (remainder));
	}
	
	str_array = g_new (char*, n + 1);
	
	str_array[n--] = NULL;
	for (slist = string_list; slist; slist = slist->next)
		str_array[n--] = slist->data;
	
	g_slist_free (string_list);
	
	return str_array;
}


static char*
g_utf8_strchug (char *string)
{
	char     *scan;
	gunichar  c;

	g_return_val_if_fail (string != NULL, NULL);
	
	scan = string;
	c = g_utf8_get_char (scan);
	while (g_unichar_isspace (c)) {
		scan = g_utf8_next_char (scan);
		c = g_utf8_get_char (scan);
	}

	g_memmove (string, scan, strlen (scan) + 1);
	
	return string;
}


static char*
g_utf8_strchomp (char *string)
{
	char   *scan;
	gsize   len;
 
	g_return_val_if_fail (string != NULL, NULL);
	
	len = g_utf8_strlen (string, -1);

	if (len == 0)
		return string;

	scan = g_utf8_offset_to_pointer (string, len - 1);

	while (len--) {
		gunichar c = g_utf8_get_char (scan);
		if (g_unichar_isspace (c))
			*scan = '\0';
		else
			break;
		scan = g_utf8_find_prev_char (string, scan);
	}
	
	return string;
}


#define g_utf8_strstrip(string)    g_utf8_strchomp (g_utf8_strchug (string))


char **
search_util_get_patterns (const char *pattern_string)
{
	char **patterns;
	int    i;
	
	patterns = g_utf8_strsplit (pattern_string, ";", MAX_PATTERNS);
	for (i = 0; patterns[i] != NULL; i++) 
		patterns[i] = g_utf8_strstrip (patterns[i]);
	
	return patterns;
}


GnomeVFSFileSize
get_dest_free_space (const char  *path)
{
	char             *escaped;
	GnomeVFSURI      *uri;
	GnomeVFSResult    result;
	GnomeVFSFileSize  ret_val;

	escaped = gnome_vfs_escape_host_and_path_string (path);
	uri = gnome_vfs_uri_new (escaped);
	g_free (escaped);

	result = gnome_vfs_get_volume_free_space (uri, &ret_val);

	gnome_vfs_uri_unref (uri);

	if (result != GNOME_VFS_OK)
		return (GnomeVFSFileSize) 0;
	else
		return ret_val;
}


#define SPECIAL_DIR(x) (! strcmp (x, "..") || ! strcmp (x, "."))


static gboolean 
path_list_new (const char  *path, 
	       GList      **files, 
	       GList      **dirs)
{
	DIR *dp;
	struct dirent *dir;
	struct stat stat_buf;
	GList *f_list = NULL;
	GList *d_list = NULL;

	dp = opendir (path);
	if (dp == NULL) return FALSE;

	while ((dir = readdir (dp)) != NULL) {
		gchar *name;
		gchar *filepath;

		/* Skip removed files */
		if (dir->d_ino == 0) 
			continue;

		name = dir->d_name;
		if (strcmp (path, "/") == 0)
			filepath = g_strconcat (path, name, NULL);
		else
			filepath = g_strconcat (path, "/", name, NULL);

		if (stat (filepath, &stat_buf) >= 0) {
			if (dirs  
			    && S_ISDIR (stat_buf.st_mode) 
			    && ! SPECIAL_DIR (name))
			{
				d_list = g_list_prepend (d_list, filepath);
				filepath = NULL;
			} else if (files && S_ISREG (stat_buf.st_mode)) {
				f_list = g_list_prepend (f_list, filepath);
				filepath = NULL;
			}
		}

		if (filepath) g_free (filepath);
	}
	closedir (dp);

	if (dirs) *dirs = g_list_reverse (d_list);
	if (files) *files = g_list_reverse (f_list);

	return TRUE;
}


gboolean
rmdir_recursive (const gchar *directory)
{
	GList    *files, *dirs;
	GList    *scan;
	gboolean  error = FALSE;

	if (! uri_is_dir (directory)) 
		return FALSE;

	path_list_new (directory, &files, &dirs);

	for (scan = files; scan; scan = scan->next) {
		char *file = scan->data;
		if ((unlink (file) < 0)) {
			g_warning ("Cannot delete %s\n", file);
			error = TRUE;
		}
	}
	path_list_free (files);

	for (scan = dirs; scan; scan = scan->next) {
		char *sub_dir = scan->data;
		if (rmdir_recursive (sub_dir) == FALSE)
			error = TRUE;
		if (rmdir (sub_dir) == 0)
			error = TRUE;
	}
	path_list_free (dirs);

	if (rmdir (directory) == 0)
		error = TRUE;

	return !error;
}


const char *
eat_spaces (const char *line)
{
	while ((*line == ' ') && (*line != 0))
		line++;
	return line;
}


char **
split_line (const char *line, 
	    int   n_fields)
{
	char       **fields;
	const char  *scan, *field_end;
	int          i;

	fields = g_new0 (char *, n_fields + 1);
	fields[n_fields] = NULL;

	scan = eat_spaces (line);
	for (i = 0; i < n_fields; i++) {
		field_end = strchr (scan, ' ');
		if (field_end != NULL) {
			fields[i] = g_strndup (scan, field_end - scan);
			scan = eat_spaces (field_end);
		}
	}

	return fields;
}


const char *
get_last_field (const char *line,
		int         last_field)
{
	const char *field;
	int         i;

	last_field--;
	field = eat_spaces (line);
	for (i = 0; i < last_field; i++) {
		field = strchr (field, ' ');
		field = eat_spaces (field);
	}

	return field;
}


char *
get_temp_work_dir (void)
{
	char temp_dir_template[] = "/tmp/goo-XXXXXX";
	g_assert (mkdtemp (temp_dir_template) != NULL);
	return g_strdup (temp_dir_template);
}


#define MAX_TRIES 50


char *
get_temp_work_dir_name (void)
{
	char       *result = NULL;
	static int  count = 0;
	int         try = 0;

	do {
		g_free (result);
		result = g_strdup_printf ("%s%s.%d.%d",
					  g_get_tmp_dir (),
					  "/goobox",
					  getpid (),
					  count++);
	} while (uri_is_file (result) && (try++ < MAX_TRIES));

	return result;
}


GnomeVFSURI *
new_uri_from_path (const char *path)
{
	char        *escaped;
	char        *uri_txt;
	GnomeVFSURI *uri;

	escaped = escape_uri (path);
	if (escaped[0] == '/')
		uri_txt = g_strconcat ("file://", escaped, NULL);
	else
		uri_txt = g_strdup (escaped);

	uri = gnome_vfs_uri_new (uri_txt);

	g_free (uri_txt);
	g_free (escaped);

	g_return_val_if_fail (uri != NULL, NULL);

	return uri;
}


char *
get_uri_from_local_path (const char *local_path)
{
        char *escaped;
        char *uri;

        escaped = escape_uri (local_path);
        if (escaped[0] == '/') {
                uri = g_strconcat ("file://", escaped, NULL);
                g_free (escaped);
        }
        else
                uri = escaped;

        return uri;
}


const char *
get_path_from_uri (const char *uri)
{
	const char *file_prefix = "file://";
	int         file_prefix_l = strlen (file_prefix);
	
	if (strncmp (uri, file_prefix, file_prefix_l) == 0)
		uri += file_prefix_l;
	return uri;
}


static gboolean
valid_filename_char (char c) 
{
	/* "$\'`\"\\!?* ()[]<>&|@#:;" */
	return strchr ("/\\!?*:;'`\"", c) == NULL;
}


/* Remove special characters from a track title in order to make it a 
 * valid filename. */
char*
tracktitle_to_filename (const char *trackname)
{
	char       *filename, *f, *f2;
	const char *t;
	gboolean    add_space;

	if (trackname == NULL)
		return NULL;

	filename = g_new (char, strlen (trackname) + 1);

	/* Substitute invalid characters with spaces. */
	f = filename;
	t = trackname;
	while (*t != 0) {
		gboolean invalid_char = FALSE;

		while ((*t != 0) && ! valid_filename_char (*t)) {
			invalid_char = TRUE;
			t++;
		}

		if (invalid_char)
			*f++ = ' ';
		
		*f = *t;

		if (*t != 0) {
			f++;
			t++;
		}
	}
	*f = 0;

	/* Remove double spaces. */
	add_space = FALSE;
	for (f = f2 = filename; *f != 0; f++)
		if (*f != ' ') {
			if (add_space) {
				*f2++ = ' ';
				add_space = FALSE;
			}
			*f2 = *f;
			f2++;
		} else
			add_space = TRUE;
	*f2 = 0;

	return filename;
}


gboolean
is_program_in_path (const char *program_name)
{
	char *str;
        int   result = FALSE;
	
        str = g_find_program_in_path (program_name);
        if (str != NULL) {
                g_free (str);
                result = TRUE;
        }
	
        return result;
}


char *
xdg_user_dir_lookup (const char *type)
{
        FILE *file;
        char *home_dir, *config_home, *config_file;
        char buffer[512];
        char *user_dir;
        char *p, *d;
        int len;
        int relative;

        home_dir = getenv ("HOME");
        if (home_dir == NULL)
                return strdup ("/tmp");

        config_home = getenv ("XDG_CONFIG_HOME");
        if (config_home == NULL || config_home[0] == 0) {
                config_file = malloc (strlen (home_dir) + strlen ("/.config/user-dirs.dirs") + 1);
                strcpy (config_file, home_dir);
                strcat (config_file, "/.config/user-dirs.dirs");
        } else {
                config_file = malloc (strlen (config_home) + strlen ("/user-dirs.dirs") + 1);
                strcpy (config_file, config_home);
                strcat (config_file, "/user-dirs.dirs");
        }

        file = fopen (config_file, "r");
        free (config_file);
        if (file == NULL)
                goto error;

        user_dir = NULL;

        while (fgets (buffer, sizeof (buffer), file)) {
                /* Remove newline at end */
                len = strlen (buffer);
                if (len > 0 && buffer[len-1] == '\n')
                buffer[len-1] = 0;
      
                p = buffer;
                while (*p == ' ' || *p == '\t')
                        p++;
      
                if (strncmp (p, "XDG_", 4) != 0)
                        continue;
                p += 4;

                if (strncmp (p, type, strlen (type)) != 0)
                        continue;
                p += strlen (type);

                if (strncmp (p, "_DIR", 4) != 0)
                continue;
                
                p += 4;

                while (*p == ' ' || *p == '\t')
                        p++;

                if (*p != '=')
                        continue;
                p++;
      
                while (*p == ' ' || *p == '\t')
                        p++;

                if (*p != '"')
                        continue;
                p++;
      
                relative = 0;
                if (strncmp (p, "$HOME/", 6) == 0) {
                        p += 6;
                        relative = 1;
                } else if (*p != '/')
                        continue;
                        if (relative) {
                        user_dir = malloc (strlen (home_dir) + 1 + strlen (p) + 1);
                        strcpy (user_dir, home_dir);
                        strcat (user_dir, "/");
                } else {
                        user_dir = malloc (strlen (p) + 1);
                        *user_dir = 0;
                }
      
                d = user_dir + strlen (user_dir);
                while (*p && *p != '"') {
                        if ((*p == '\\') && (*(p+1) != 0))
                                p++;
                        *d++ = *p++;
                }
                *d = 0;
        }  
        
        fclose (file);

        if (user_dir)
                return user_dir;

error:
         /* Special case desktop for historical compatibility */
        if (strcmp (type, "DESKTOP") == 0) {
                user_dir = malloc (strlen (home_dir) + strlen ("/Desktop") + 1);
                strcpy (user_dir, home_dir);
                strcat (user_dir, "/Desktop");
                return user_dir;
        } else
                return strdup (home_dir);
}
