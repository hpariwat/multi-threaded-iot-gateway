#ifndef _ERRMACROS_H_
#define _ERRMACROS_H_

#include <errno.h>

#define LINE_PRINT() fprintf(stderr, "\n\033[31m[ERROR]\033[0m In %s - function %s at line %d:\n", __FILE__, __func__, __LINE__);

// print debug statement
#ifdef DEBUG
	#define DEBUG_PRINTF(...) \
		do { \
            fprintf(stderr, "\033[33m[DEBUG]\033[0m "); \
			fprintf(stderr, __VA_ARGS__); \
			fflush(stderr); \
        } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif

// print the string into FIFO
#define LOG_PRINTF(...) \
	do { \
        char log[LOG_LENGTH]; \
        snprintf(log, LOG_LENGTH, __VA_ARGS__); \
        log_write(log); \
    } while(0)

// general errors with no condition
#define ERROR_PRINTF(...) \
    do { \
        LINE_PRINT(); \
        printf(__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while(0)

// general erros with condition
#define ERROR_HANDLER(condition, ...) \
    do { \
        if (condition) { \
			LINE_PRINT(); \
	        printf("Error: %s\n", __VA_ARGS__); \
	        exit(EXIT_FAILURE); \
        } \
    } while(0)

// call realloc with safe procedure
#define REALLOC_CHECK(poll_fd, size) \
    do { \
        poll_fd_t* dummy = realloc(poll_fd, size); \
        if (dummy == NULL) { \
			free(poll_fd); \
			LINE_PRINT(); \
			fprintf(stderr, "Realloc error\n"); \
			exit(EXIT_FAILURE); \
		} \
        poll_fd = dummy; \
    } while(0)

// errors when calling malloc or calloc
#define ALLOC_ERR(dummy) \
	do { \
		if (dummy == NULL) { \
			LINE_PRINT(); \
			fprintf(stderr, "Memory allocation error\n"); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors when calling asprintf
#define ASPRINTF_ERR(rc) \
	do { \
		if (rc == -1) { \
			perror("Asprintf error"); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors regarding TCP
#define TCP_ERR(rc) \
    do { \
        if (rc != TCP_NO_ERROR) { \
            LINE_PRINT(); \
            printf("TCP error code %d\n", rc); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// errors regarding pthreads
#define PTHR_ERR(rc) \
    do { \
		if (rc != 0) { \
			LINE_PRINT(); \
			fprintf(stderr, "Error Code %d\n", rc); \
	        perror("POSIX Thread error"); \
	        exit(EXIT_FAILURE); \
		} \
    } while(0)

// errors regarding system calls
#define SYS_ERR(rc) \
	do { \
		if (rc == -1) { \
			LINE_PRINT(); \
			fprintf(stderr, "Error Code %d\n", rc); \
			perror("Syscall Error"); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors regarding creation of files
#define FILE_OPEN_ERR(fp, filename) \
	do { \
		if (fp == NULL)	{ \
			LINE_PRINT(); \
			fprintf(stderr, "Error opening file '%s'\n", filename); \
			perror(NULL); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors regarding destruction of files
#define FILE_CLOSE_ERR(fp, filename) \
	do { \
		if (fp == NULL)	{ \
			LINE_PRINT(); \
			fprintf(stderr, "Error closing file '%s'\n", filename); \
			perror(NULL); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors regarding shared buffers
#define SBUFFER_ERR(rc) \
	do { \
		if (rc == SBUFFER_FAILURE) { \
			LINE_PRINT(); \
			fprintf(stderr, "Sbuffer error code %d\n", rc); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

// errors regarding creation of FIFO files
#define MKFIFO_ERR(rc) \
	do { \
		if (rc == -1) { \
			if ( errno != EEXIST ) { \
				LINE_PRINT(); \
				fprintf(stderr, "Error Code %d\n", rc); \
				perror("MKFIFO error"); \
				exit(EXIT_FAILURE); \
			} \
		} \
	} while(0)

// errors regarding pthread barriers
#define BARRIER_ERR(rc) \
    do { \
		if (rc == EINVAL) { \
			LINE_PRINT(); \
			fprintf(stderr, "Error Code %d\n", rc); \
	        perror("POSIX Thread error"); \
	        exit(EXIT_FAILURE); \
		} \
    } while(0)

#endif
