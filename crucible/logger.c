/**
 * Logging Framework Implementation
 *
 * This file implements a simple but powerful logging system for recording
 * test activities, metrics, and errors. It supports session logs with
 * different severity levels and metric logs in CSV format.
 *
 * Author: Your Name
 * Date: March 20, 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Include our header file */
#include "logger.h"

/* Define constants */
#define MAX_LOG_LINE_LENGTH 1024
#define MAX_TIMESTAMP_LENGTH 64
#define BYTES_PER_MB (1024 * 1024)

/* Global logger instance that will be used throughout the program */
Logger g_logger = {NULL, NULL, NULL, LOG_INFO, false, 0, true, 0};

/* Private helper function prototypes */
static bool create_directory(const char *path);
static char *get_timestamp(char *buffer, size_t size, bool include_date);
static bool open_log_files(void);
static bool check_and_rotate_logs(void);
static size_t get_file_size(FILE *file);

/**
 * Initialize the logging system
 *
 * Sets up the logger structure, creates log directory if needed,
 * and opens log files.
 */
bool logger_init(const char *log_dir, LogLevel level, unsigned int rotate_mb, bool buffer)
{
    /* Don't initialize twice */
    if (g_logger.initialized)
    {
        fprintf(stderr, "Logger already initialized\n");
        return false;
    }

    /* Store the start time */
    g_logger.start_time = time(NULL);

    /* Set the log level */
    g_logger.level = level;

    /* Set buffer mode */
    g_logger.buffer_enabled = buffer;

    /* Set max file size for rotation (convert MB to bytes) */
    g_logger.max_file_size = rotate_mb * BYTES_PER_MB;

    /* Create or store log directory */
    if (log_dir != NULL && strlen(log_dir) > 0)
    {
        /* Create a copy of the log directory */
        g_logger.log_dir = strdup(log_dir);
        if (!g_logger.log_dir)
        {
            fprintf(stderr, "Failed to allocate memory for log directory\n");
            return false;
        }

        /* Create the directory if it doesn't exist */
        if (!create_directory(g_logger.log_dir))
        {
            fprintf(stderr, "Failed to create log directory: %s\n", g_logger.log_dir);
            free(g_logger.log_dir);
            g_logger.log_dir = NULL;
            return false;
        }
    }
    else
    {
        /* Use the current directory */
        g_logger.log_dir = strdup(".");
        if (!g_logger.log_dir)
        {
            fprintf(stderr, "Failed to allocate memory for log directory\n");
            return false;
        }
    }

    /* Open log files */
    if (!open_log_files())
    {
        fprintf(stderr, "Failed to open log files\n");
        free(g_logger.log_dir);
        g_logger.log_dir = NULL;
        return false;
    }

    /* Write headers to the metrics log */
    fprintf(g_logger.metric_log, "timestamp,elapsed_seconds,metric,values\n");

    /* Mark as initialized */
    g_logger.initialized = true;

    /* Log that we've started */
    logger_info("Logging initialized (level: %s, directory: %s, rotation: %u MB, buffering: %s)",
                logger_level_str(level),
                g_logger.log_dir,
                rotate_mb,
                buffer ? "enabled" : "disabled");

    return true;
}

/**
 * Clean up the logging system
 */
void logger_cleanup(void)
{
    if (!g_logger.initialized)
    {
        return;
    }

    /* Log that we're shutting down */
    logger_info("Logging system shutting down");

    /* Flush any pending writes */
    logger_flush();

    /* Close files */
    if (g_logger.session_log != NULL)
    {
        fclose(g_logger.session_log);
        g_logger.session_log = NULL;
    }

    if (g_logger.metric_log != NULL)
    {
        fclose(g_logger.metric_log);
        g_logger.metric_log = NULL;
    }

    /* Free memory */
    free(g_logger.log_dir);
    g_logger.log_dir = NULL;

    /* Mark as uninitialized */
    g_logger.initialized = false;
}

/**
 * Set the current log level
 */
void logger_set_level(LogLevel level)
{
    if (!g_logger.initialized)
    {
        return;
    }

    /* Log the change */
    logger_info("Changing log level from %s to %s",
                logger_level_str(g_logger.level),
                logger_level_str(level));

    /* Update the level */
    g_logger.level = level;
}

/**
 * Convert a log level to a string representation
 */
const char *logger_level_str(LogLevel level)
{
    switch (level)
    {
    case LOG_DEBUG:
        return "DEBUG";
    case LOG_INFO:
        return "INFO";
    case LOG_WARNING:
        return "WARNING";
    case LOG_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

/**
 * Write a message to the session log
 */
void logger_log(LogLevel level, const char *message, ...)
{
    /* Check if we're initialized and if we should log this message */
    if (!g_logger.initialized || level < g_logger.level)
    {
        return;
    }

    /* Check and rotate logs if needed */
    check_and_rotate_logs();

    /* Format the timestamp */
    char timestamp[MAX_TIMESTAMP_LENGTH];
    get_timestamp(timestamp, sizeof(timestamp), true);

    /* Format the message with variable arguments */
    char formatted_message[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    /* Write to the log file */
    fprintf(g_logger.session_log, "[%s] [%s] %s\n",
            timestamp,
            logger_level_str(level),
            formatted_message);

    /* Flush if we're not buffering or it's an error */
    if (!g_logger.buffer_enabled || level == LOG_ERROR)
    {
        fflush(g_logger.session_log);
    }
}

/**
 * Debug level logging
 */
void logger_debug(const char *message, ...)
{
    if (!g_logger.initialized || LOG_DEBUG < g_logger.level)
    {
        return;
    }

    /* Format the message with variable arguments */
    char formatted_message[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    /* Use the main logging function */
    logger_log(LOG_DEBUG, "%s", formatted_message);
}

/**
 * Info level logging
 */
void logger_info(const char *message, ...)
{
    if (!g_logger.initialized || LOG_INFO < g_logger.level)
    {
        return;
    }

    /* Format the message with variable arguments */
    char formatted_message[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    /* Use the main logging function */
    logger_log(LOG_INFO, "%s", formatted_message);
}

/**
 * Warning level logging
 */
void logger_warning(const char *message, ...)
{
    if (!g_logger.initialized || LOG_WARNING < g_logger.level)
    {
        return;
    }

    /* Format the message with variable arguments */
    char formatted_message[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    /* Use the main logging function */
    logger_log(LOG_WARNING, "%s", formatted_message);
}

/**
 * Error level logging
 */
void logger_error(const char *message, ...)
{
    if (!g_logger.initialized || LOG_ERROR < g_logger.level)
    {
        return;
    }

    /* Format the message with variable arguments */
    char formatted_message[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    /* Use the main logging function */
    logger_log(LOG_ERROR, "%s", formatted_message);
}

/**
 * Write a record to the metrics log
 */
void logger_metric(const char *metric_name, const char *format, ...)
{
    /* Check if we're initialized */
    if (!g_logger.initialized)
    {
        return;
    }

    /* Check and rotate logs if needed */
    check_and_rotate_logs();

    /* Format the timestamp */
    char timestamp[MAX_TIMESTAMP_LENGTH];
    get_timestamp(timestamp, sizeof(timestamp), true);

    /* Calculate elapsed seconds */
    time_t now = time(NULL);
    double elapsed = difftime(now, g_logger.start_time);

    /* Format the values with variable arguments */
    char values[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(values, sizeof(values), format, args);
    va_end(args);

    /* Write to the metrics log file (in CSV format) */
    fprintf(g_logger.metric_log, "%s,%.1f,%s,%s\n",
            timestamp,
            elapsed,
            metric_name,
            values);

    /* Flush if we're not buffering */
    if (!g_logger.buffer_enabled)
    {
        fflush(g_logger.metric_log);
    }
}

/**
 * Force writing buffered log data to disk
 */
bool logger_flush(void)
{
    if (!g_logger.initialized)
    {
        return false;
    }

    /* Flush both log files */
    bool session_ok = (fflush(g_logger.session_log) == 0);
    bool metric_ok = (fflush(g_logger.metric_log) == 0);

    return session_ok && metric_ok;
}

/**
 * Get the current log directory
 */
const char *logger_get_directory(void)
{
    if (!g_logger.initialized)
    {
        return NULL;
    }

    return g_logger.log_dir;
}

/**
 * Rotate the log files
 */
bool logger_rotate(void)
{
    if (!g_logger.initialized)
    {
        return false;
    }

    /* Get current time for the rotation timestamp */
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char timestamp[MAX_TIMESTAMP_LENGTH];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", time_info);

    /* Flush any pending writes */
    logger_flush();

    /* Close the current log files */
    if (g_logger.session_log != NULL)
    {
        fclose(g_logger.session_log);
        g_logger.session_log = NULL;
    }

    if (g_logger.metric_log != NULL)
    {
        fclose(g_logger.metric_log);
        g_logger.metric_log = NULL;
    }

    /* Construct paths for the current log files */
    char session_path[1024];
    char metric_path[1024];
    snprintf(session_path, sizeof(session_path), "%s/session.log", g_logger.log_dir);
    snprintf(metric_path, sizeof(metric_path), "%s/metrics.csv", g_logger.log_dir);

    /* Construct paths for the archived log files */
    char archived_session_path[1024];
    char archived_metric_path[1024];
    snprintf(archived_session_path, sizeof(archived_session_path),
             "%s/session_%s.log", g_logger.log_dir, timestamp);
    snprintf(archived_metric_path, sizeof(archived_metric_path),
             "%s/metrics_%s.csv", g_logger.log_dir, timestamp);

    /* Rename the current log files */
    if (rename(session_path, archived_session_path) != 0)
    {
        /* It's okay if the file doesn't exist yet */
        if (errno != ENOENT)
        {
            fprintf(stderr, "Failed to rename session log file\n");
            return false;
        }
    }

    if (rename(metric_path, archived_metric_path) != 0)
    {
        /* It's okay if the file doesn't exist yet */
        if (errno != ENOENT)
        {
            fprintf(stderr, "Failed to rename metric log file\n");
            return false;
        }
    }

    /* Open new log files */
    if (!open_log_files())
    {
        fprintf(stderr, "Failed to open new log files after rotation\n");
        return false;
    }

    /* Write headers to the metrics log */
    fprintf(g_logger.metric_log, "timestamp,elapsed_seconds,metric,values\n");

    /* Log that we rotated the logs */
    logger_info("Log files rotated");

    return true;
}

/* Private helper function to create a directory */
static bool create_directory(const char *path)
{
    struct stat st;

    /* Check if directory already exists */
    if (stat(path, &st) == 0)
    {
        /* Check if it's a directory */
        if (S_ISDIR(st.st_mode))
        {
            return true; /* Directory exists */
        }
        else
        {
            return false; /* Path exists but isn't a directory */
        }
    }

    /* Create the directory */
#ifdef _WIN32
    /* Windows specific directory creation */
    if (mkdir(path) != 0)
    {
#else
    /* POSIX directory creation with permissions */
    if (mkdir(path, 0755) != 0)
    {
#endif
        return false;
    }

    return true;
}

/* Private helper function to get a formatted timestamp */
static char *get_timestamp(char *buffer, size_t size, bool include_date)
{
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);

    if (include_date)
    {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", time_info);
    }
    else
    {
        strftime(buffer, size, "%H:%M:%S", time_info);
    }

    return buffer;
}

/* Private helper function to open log files */
static bool open_log_files(void)
{
    /* Construct file paths */
    char session_path[1024];
    char metric_path[1024];
    snprintf(session_path, sizeof(session_path), "%s/session.log", g_logger.log_dir);
    snprintf(metric_path, sizeof(metric_path), "%s/metrics.csv", g_logger.log_dir);

    /* Open the session log file */
    g_logger.session_log = fopen(session_path, "a");
    if (g_logger.session_log == NULL)
    {
        return false;
    }

    /* Open the metrics log file */
    g_logger.metric_log = fopen(metric_path, "a");
    if (g_logger.metric_log == NULL)
    {
        fclose(g_logger.session_log);
        g_logger.session_log = NULL;
        return false;
    }

    /* Set buffering mode */
    if (!g_logger.buffer_enabled)
    {
        /* Disable buffering for immediate writes */
        setvbuf(g_logger.session_log, NULL, _IONBF, 0);
        setvbuf(g_logger.metric_log, NULL, _IONBF, 0);
    }
    else
    {
        /* Use line buffering for a good compromise */
        setvbuf(g_logger.session_log, NULL, _IOLBF, 0);
        setvbuf(g_logger.metric_log, NULL, _IOLBF, 0);
    }

    return true;
}

/* Private helper function to check and rotate logs if needed */
static bool check_and_rotate_logs(void)
{
    /* Skip if rotation is disabled or not initialized */
    if (!g_logger.initialized || g_logger.max_file_size == 0)
    {
        return true;
    }

    /* Check file sizes */
    size_t session_size = get_file_size(g_logger.session_log);
    size_t metric_size = get_file_size(g_logger.metric_log);

    /* Rotate if either file exceeds the limit */
    if (session_size > g_logger.max_file_size || metric_size > g_logger.max_file_size)
    {
        return logger_rotate();
    }

    return true;
}

/* Private helper function to get file size */
static size_t get_file_size(FILE *file)
{
    if (file == NULL)
    {
        return 0;
    }

    /* Remember current position */
    long current_pos = ftell(file);
    if (current_pos < 0)
    {
        return 0;
    }

    /* Seek to end and get position (file size) */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        return 0;
    }

    long size = ftell(file);

    /* Restore original position */
    fseek(file, current_pos, SEEK_SET);

    return (size > 0) ? (size_t)size : 0;
}