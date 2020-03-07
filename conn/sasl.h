/**
 * @file
 * SASL authentication support
 *
 * @authors
 * Copyright (C) 2000-2005,2008 Brendan Cully <brendan@kublai.com>
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

/* common SASL helper routines */

#ifndef MUTT_CONN_SASL_H
#define MUTT_CONN_SASL_H

#include <stddef.h>
#include <sasl/sasl.h>
#include <time.h>

struct Connection;

int mutt_sasl_client_new(struct Connection *conn, sasl_conn_t **saslconn);
int mutt_sasl_interact(sasl_interact_t *interaction);
void mutt_sasl_setup_conn(struct Connection *conn, sasl_conn_t *saslconn);
void mutt_sasl_done(void);

/**
 * struct SaslSockData - SASL authentication API
 */
struct SaslSockData
{
  sasl_conn_t *saslconn;
  const sasl_ssf_t *ssf;
  const unsigned int *pbufsize;

  /* read buffer */
  const char *buf;
  unsigned int blen;
  unsigned int bpos;

  void *sockdata; ///< Underlying socket data

  /**
   * open - Open a socket Connection - Implements Connection::open()
   */
  int (*open) (struct Connection *conn);

  /**
   * read - Read from a socket Connection - Implements Connection::read()
   */
  int (*read) (struct Connection *conn, char *buf, size_t count);

  /**
   * write - Write to a socket Connection - Implements Connection::write()
   */
  int (*write)(struct Connection *conn, const char *buf, size_t count);

  /**
   * poll - Check whether a socket read would block - Implements Connection::poll()
   */
  int (*poll) (struct Connection *conn, time_t wait_secs);

  /**
   * close - Close a socket Connection - Implements Connection::close()
   */
  int (*close)(struct Connection *conn);
};

#endif /* MUTT_CONN_SASL_H */
