/**
 * Logging Framework Header
 *
 * This header file defines the interface for a simple but powerful logging system
 * that supports different log levels, session logs, and metric logs in CSV format.
 *
 * Author: Your Name
 * Date: March 20, 2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <time.h>

/**
 * Log Levels:
 * These define the severity/importance of log messages.
 * They are ordered from most verbose (DEBUG) to least verbose (ERROR).
 */
typedef enum
{
    LOG_DEBUG,   /* Detailed information, useful for debugging */
    LOG_INFO,    /* General information about program operation */
    LOG_WARNING, /* Warning conditions (not errors, but might need attention) */
    LOG_ERROR    /* Error conditions that prevent normal operation */
} LogLevel;

/**
 * Logger Structure:
 * Holds the state of the logger, including file handles and settings.
 * Users shouldn't access these fields directly - use the provided functions.
 */
typedef struct
{
    FILE *session_log;    /* File handle for the session log */
    FILE *metric_log;     /* File handle for the metrics log */
    char *log_dir;        /* Directory where logs are stored */
    LogLevel level;       /* Current log level */
    bool initialized;     /* Flag indicating if logger is initialized */
    time_t start_time;    /* When the logger was first initialized */
    bool buffer_enabled;  /* Whether to buffer writes for performance */
    size_t max_file_size; /* Maximum size for log files (for rotation) */
} Logger;

/**
 * Initialize the logging system
 *
 * Sets up log files, creates directories if needed, and prepares the logging
 * system. This must be called before any other logging functions.
 *
 * Parameters:
 *   log_dir   - Directory to store log files (NULL for current directory)
 *   level     - Initial log level
 *   rotate_mb - Size in MB at which to rotate log files (0 to disable rotation)
 *   buffer    - Whether to buffer log writes for performance
 *
 * Returns:
 *   true if initialization successful, false otherwise
 */
bool logger_init(const char *log_dir, LogLevel level, unsigned int rotate_mb, bool buffer);

/**
 * Clean up the logging system
 *
 * Closes open files and releases resources. Should be called before
 * the program exits.
 */
void logger_cleanup(void);

/**
 * Set the current log level
 *
 * Changes what messages will be recorded. Only messages at the specified
 * level or higher (more severe) will be logged.
 *
 * Parameters:
 *   level - New log level
 */
void logger_set_level(LogLevel level);

/**
 * Convert a log level to a string representation
 *
 * Useful for displaying the log level in messages.
 *
 * Parameters:
 *   level - Log level to convert
 *
 * Returns:
 *   String representation of the log level
 */
const char *logger_level_str(LogLevel level);

/**
 * Write a message to the session log
 *
 * Records a message with the specified log level in the session log.
 * Messages below the current log level will be ignored.
 *
 * Parameters:
 *   level   - Severity of the message
 *   message - Format string (like printf)
 *   ...     - Additional arguments for the format string
 */
void logger_log(LogLevel level, const char *message, ...);

/**
 * Debug level logging
 *
 * Convenience function for debug-level messages. Equivalent to:
 * logger_log(LOG_DEBUG, message, ...)
 */
void logger_debug(const char *message, ...);

/**
 * Info level logging
 *
 * Convenience function for info-level messages. Equivalent to:
 * logger_log(LOG_INFO, message, ...)
 */
void logger_info(const char *message, ...);

/**
 * Warning level logging
 *
 * Convenience function for warning-level messages. Equivalent to:
 * logger_log(LOG_WARNING, message, ...)
 */
void logger_warning(const char *message, ...);

/**
 * Error level logging
 *
 * Convenience function for error-level messages. Equivalent to:
 * logger_log(LOG_ERROR, message, ...)
 */
void logger_error(const char *message, ...);

/**
 * Write a record to the metrics log
 *
 * Records a set of key-value pairs to the metrics log in CSV format.
 * This is useful for data that will be analyzed later.
 *
 * Parameters:
 *   metric_name - Name of the metric being logged
 *   format      - Format string for additional values
 *   ...         - Additional arguments for the format string
 *
 * Example:
 *   logger_metric("cpu_usage", "%.2f,%.2f,%.2f", user, system, idle);
 */
void logger_metric(const char *metric_name, const char *format, ...);

/**
 * Force writing buffered log data to disk
 *
 * If buffering is enabled, this ensures all pending log data is written to disk.
 * Useful to call before checking log files or in error conditions.
 *
 * Returns:
 *   true if successful, false otherwise
 */
bool logger_flush(void);

/**
 * Get the current log directory
 *
 * Returns the full path to the directory where logs are being written.
 *
 * Returns:
 *   String containing the log directory path
 */
const char *logger_get_directory(void);

/**
 * Rotate the log files
 *
 * Forces log rotation, which closes the current log files, renames them
 * with a timestamp, and opens new files. This is typically done automatically
 * based on file size, but can be triggered manually with this function.
 *
 * Returns:
 *   true if rotation successful, false otherwise
 */
bool logger_rotate(void);

/* Global logger instance */
extern Logger g_logger;

#endif /* LOGGER_H */