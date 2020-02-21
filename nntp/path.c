/**
 * @file
 * Nntp path manipulations
 *
 * @authors
 * Copyright (C) 2020 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page nntp_path Nntp path manipulations
 *
 * Nntp path manipulations
 */

#include "config.h"
#include <string.h>
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"

extern char *HomeDir;

/**
 * nntp_path2_canon - Canonicalise a Mailbox path - Implements MxOps::path2_canon()
 * @param path Path to canonicalise
 * @param user Login username
 * @param port Server port
 * @retval  0 Success
 * @retval -1 Failure
 */
int nntp_path2_canon(struct Path *path, char *user, int port)
{
  struct Url *url = url_parse(path->orig);
  if (!url)
    return -1;

  if (!url->user)
    url->user = user;
  if (url->port == 0)
    url->port = port;
  if (url->pass)
    url->pass = NULL;

  int rc = -1;
  struct Buffer buf = mutt_buffer_make(256);
  if (url_tobuffer(url, &buf, 0) != 0)
    goto done; // LCOV_EXCL_LINE

  mutt_str_replace(&path->canon, mutt_b2s(&buf));
  path->flags |= MPATH_CANONICAL;
  rc = 0;

done:
  url_free(&url);
  mutt_buffer_dealloc(&buf);
  return rc;
}

/**
 * nntp_path2_compare - Compare two Mailbox paths - Implements MxOps::path2_compare()
 *
 * **Tests**
 * - scheme must match
 * - host must match
 * - user must match, or may be absent from one, or absent from both
 * - pass must match, or may be absent from one, or absent from both
 * - port must match, or may be absent from one, or absent from both
 * - path must match
 */
int nntp_path2_compare(struct Path *path1, struct Path *path2)
{
  struct Url *url1 = url_parse(path1->canon);
  struct Url *url2 = url_parse(path2->canon);

  int rc = url1->scheme - url2->scheme;
  if (rc != 0)
    goto done;

  if (url1->user && url2->user)
  {
    rc = mutt_str_strcmp(url1->user, url2->user);
    if (rc != 0)
      goto done;
  }

  rc = mutt_str_strcasecmp(url1->host, url2->host);
  if (rc != 0)
    goto done;

  if ((url1->port != 0) && (url2->port != 0))
  {
    rc = url1->port - url2->port;
    if (rc != 0)
      goto done;
  }

  rc = mutt_str_strcmp(url1->path, url2->path);

done:
  url_free(&url1);
  url_free(&url2);
  if (rc < 0)
    return -1;
  if (rc > 0)
    return 1;
  return 0;
}

/**
 * nntp_path2_parent - Find the parent of a Mailbox path - Implements MxOps::path2_parent()
 * @retval  1 Success, path is root, it has no parent
 * @retval  0 Success, parent returned
 * @retval -1 Error
 */
int nntp_path2_parent(const struct Path *path, struct Path **parent)
{
  int rc = -1;
  struct Buffer *buf = NULL;

  struct Url *url = url_parse(path->orig);
  if (!url)
    return -2;

  char *split = strrchr(url->path, '.');
  if (!split)
    goto done;

  *split = '\0';

  buf = mutt_buffer_pool_get();
  if (url_tobuffer(url, buf, 0) < 0)
    goto done; // LCOV_EXCL_LINE

  *parent = mutt_path_new();
  (*parent)->orig = mutt_str_strdup(mutt_b2s(buf));
  (*parent)->type = path->type;
  (*parent)->flags = MPATH_RESOLVED | MPATH_TIDY;

  rc = 0;

done:
  url_free(&url);
  mutt_buffer_pool_release(&buf);
  return rc;
}

/**
 * nntp_path2_pretty - Abbreviate a Mailbox path - Implements MxOps::path2_pretty()
 *
 * **Tests**
 * - scheme must match
 * - host must match
 * - user must match, or may be absent from one, or absent from both
 * - pass must match, or may be absent from one, or absent from both
 * - port must match, or may be absent from one, or absent from both
 */
int nntp_path2_pretty(const struct Path *path, const char *folder, char **pretty)
{
  int rc = 0;
  struct Url *url1 = url_parse(path->orig);
  struct Url *url2 = url_parse(folder);

  if (url1->scheme != url2->scheme)
    goto done;
  if (mutt_str_strcasecmp(url1->host, url2->host) != 0)
    goto done;
  if (!path_partial_match_string(url1->user, url2->user))
    goto done;
  if (!path_partial_match_number(url1->port, url2->port))
    goto done;
  if (!mutt_path2_abbr_folder(url1->path, url2->path, pretty))
    goto done;

  rc = 1;

done:
  url_free(&url1);
  url_free(&url2);
  return rc;
}

/**
 * nntp_path2_probe - Does this Mailbox type recognise this path? - Implements MxOps::path2_probe()
 *
 * **Tests**
 * - Path may begin "news://"
 * - Path may begin "snews://"
 *
 * @note The case of the URL scheme is ignored
 */
int nntp_path2_probe(struct Path *path, const struct stat *st)
{
  if (!mutt_str_startswith(path->orig, "news://", CASE_IGNORE) &&
      !mutt_str_startswith(path->orig, "snews://", CASE_IGNORE))
  {
    return -1;
  }

  path->type = MUTT_NNTP;
  return 0;
}

/**
 * nntp_path2_tidy - Tidy a Mailbox path - Implements MxOps::path2_tidy()
 *
 * **Changes**
 * - Lowercase the URL scheme
 */
int nntp_path2_tidy(struct Path *path)
{
  struct Url *url = url_parse(path->orig);
  if (!url)
    return -1;

  url->pass = NULL;

  int rc = -1;
  struct Buffer *buf = mutt_buffer_pool_get();
  if (url_tobuffer(url, buf, 0) < 0)
    goto done; // LCOV_EXCL_LINE

  mutt_str_replace(&path->orig, mutt_b2s(buf));
  path->flags |= MPATH_TIDY;
  rc = 0;

done:
  url_free(&url);
  mutt_buffer_pool_release(&buf);
  return rc;
}