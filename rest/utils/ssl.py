import os
import subprocess
from tempfile import mkstemp

SSL_DIR = "/etc/ssl"
SSL_PRIV_DIR = "/etc/ssl/private"
SSL_PRIV_KEY_FILE = "/etc/ssl/private/server-private.key"
SSL_CRT_DIR = "/etc/ssl/certs"
SSL_CRT_FILE = "/etc/ssl/certs/server.crt"

def create_ssl_pki():
    if not os.path.exists(SSL_DIR):
        os.mkdir(SSL_DIR)

    if not os.path.exists(SSL_PRIV_DIR):
        os.mkdir(SSL_PRIV_DIR)

    if not os.path.exists(SSL_CRT_DIR):
        os.mkdir(SSL_CRT_DIR)

    if os.path.isfile(SSL_CRT_FILE) and os.path.isfile(SSL_PRIV_KEY_FILE):
        return

    subprocess.call(['openssl', 'genrsa', '-out', SSL_PRIV_KEY_FILE,
                     '2048'])

    fd, ssl_csr_file = mkstemp()
    subprocess.call(['openssl', 'req', '-new', '-key',
                     SSL_PRIV_KEY_FILE, '-out',
                     ssl_csr_file, '-subj',
                     '/C=CN/ST=JS/L=SZ/CN=AsterFusion/O=Tapplet/OU=REST/emailAddress=support@Asterfusion.com'])

    subprocess.call(['openssl', 'x509', '-req', '-days', '14600', '-in',
                     ssl_csr_file, '-signkey',
                     SSL_PRIV_KEY_FILE, '-out',
                     SSL_CRT_FILE])
    os.close(fd)
    os.unlink(ssl_csr_file)
