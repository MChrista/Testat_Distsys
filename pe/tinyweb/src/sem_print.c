/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>


#define SEM_NAME                  "/tinysem"
#define err_print(s)              fprintf(stderr, "ERROR: %s, %s:%d\n", (s), __FILE__, __LINE__)


static sem_t *log_sem = NULL;
static unsigned short verbosity_level = 0;

void
init_logging_semaphore(void)
{
    if ((log_sem = sem_open(SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        err_print("cannot create named semaphore");
        exit(EXIT_FAILURE);
    } /* end if */
} /* end of set_logging_semaphore */


void
set_verbosity_level(unsigned short level)
{
    verbosity_level = level;
} /* end of set_verbosity_level */


int
print_log(const char *format, ...)
{
    int status;
    va_list args;

    if (log_sem != NULL && sem_wait(log_sem) < 0) {
        err_print("semaphore wait");
    } /* end if */

    printf("[%d] ", getpid());

    va_start(args, format);
    status = vprintf(format, args);
    va_end(args);

    if (log_sem != NULL && sem_post(log_sem) < 0) {
        err_print("semaphore post");
    } /* end if */

    return status;
} /* end of print_log */


#define MAXS 1024
int
print_debug(const char *format, ...)
{
    int status;
    int pos;
    va_list args;
    char buf[MAXS];

    if (verbosity_level < 2) {
        return 0;
    } /* end if */

    if (log_sem != NULL && sem_wait(log_sem) < 0) {
        err_print("semaphore wait");
    } /* end if */

    pos = snprintf(buf, sizeof(buf), "[%d] ", getpid());

    va_start(args, format);
    vsnprintf(buf+pos, sizeof(buf)-pos, format, args);
    va_end(args);
    status = write(STDOUT_FILENO, buf, strlen(buf)); /* write is async-signal-safe */

    if (log_sem != NULL && sem_post(log_sem) < 0) {
        err_print("semaphore post");
    } /* end if */

    return status;
} /* end of print_debug */


void
print_http_header(const char *what, const char *response_str)
{
    char *ptr;
    char *prev_ptr;
    char *buffer;
    size_t len;

    if (verbosity_level == 0) {
        return;
    } /* end if */

    /* allocate a separate buffer and copy into it the response string
     * because we need to add EOS characters into the string in order
     * to separate the individual response header lines */
    len = strlen(response_str);
    buffer = (char *)malloc(len + 1);
    if (buffer != NULL) {
        strcpy(buffer, response_str);
    } else {
        err_print("cannot allocate memory");
        return;
    } /* end if */

    if (log_sem != NULL && sem_wait(log_sem) < 0) {
        err_print("semaphore wait");
    } /* end if */

    printf("[%d] %s HEADER (%zd Bytes):\n", getpid(), what, len);

    prev_ptr = buffer;
    while (1) {
        if ((ptr = strstr(prev_ptr, "\r\n")) == NULL) {
            break;
        } /* end if */

        *ptr = '\0';
        printf("  %s\n", prev_ptr);
        prev_ptr = ptr+2;   // Note: strlen("\r\n") = 2
    } /* end while */

    if (log_sem != NULL && sem_post(log_sem) < 0) {
        err_print("semaphore post");
    } /* end if */

    free(buffer);
} /* end of print_http_header */

