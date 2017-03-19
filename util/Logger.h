#ifndef _Logger_h_
#define _Logger_h_

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

#include <string>

#include "Export.h"


/** \file
    \brief The logging system consists of named loggers, with levels using
    OptionsDB for configuration.

    The system is a thin wrapper around boost::log.

    The logging system is composed of a sink which writes to the log files and
    sources, called loggers, which collect the log information while the
    application is running.

    Both the sinks and the sources use debug levels (debug, info, warn and
    error) to filter which log records are generated at the sources and which
    log records are written at the sinks. Logs that are filtered out at either
    the sink or the source level are not generated by the source.

    The intended uses of the levels are:
    error - used for "major" unrecoverable errors which will affect game play.  Error level issues
            need to be fixed.  Error level will probably not be turned off unless they are flooding
            the logs.
            Examples are: the game is about to crash, a string is missing from the stringtable, etc.
    warn  - used for "minor", recoverable errors that will not affect game play but do indicate a
            problem.
            For example a missing id that can be ignored, an extra item in a container.
    info  - used to report normal game state and progress.  This should be the default level of
            logging.  The number of log should be low enought to not floow the logs.
            For example reporting that the network connected/disconnected.
    debug - used for low-level implementation or calculation details.  For a named logger this
            level will probably only be turned on by devs working on that
            section of code.  This will be detailed and perhaps voluminous.
            For example reporting that the network disconnected via a client initiated shutdown
            with a linger time of 30 ms before closing.


    Sources are further filtered by name.  For example "combat" is the name of the combat system's logger.

    Usage:

    The default loggers as stream outputs, like std::cout as follow:

    DebugLogger() << "Put any streamable output here.  It will only be computed if debug
                  <<  " level is turned on in the log file";
    InfoLogger()  << "Put any streamable output here.  It will only be computed if info
                  <<  " level is turned on in the log file";
    WarnLogger()  << "Put any streamable output here.  It will only be computed if warn
                  <<  " level is turned on in the log file";
    ErrorLogger() << "Put any streamable output here.  It will only be computed if error
                  <<  " level is turned on in the log file";


    The named loggers are created with:
    CreateThreadedLogger(name_of_logger);

    And used with

    DebugLogger(name_of_logger) << "streamable output, only computed if debug is turned on"
                                << "in the log file AND the logger called name_of_logger";
    InfoLogger(name_of_logger)  << "streamable output";
    WarnLogger(name_of_logger)  << "streamable output";
    ErrorLogger(name_of_logger) << "streamable output";

    Enabling/disabling the sinks and sources is controlled statically on startup
    through OptionsDB, with either the files config.xml and/or
    persistent_config.xml or dynamically with the app options window.

    The logging section in the configuration file looks like:

    <logging>
      <sinks>
        <client>info</client>
        <server>debug</server>
        <ai>debug</ai>
      </sinks>
      <sources>
        <combat>debug</combat>
        <combat-log>info</combat-log>
        <network>warn</network>
        <ai>debug</ai>
      </sources>
    </logging>

    The <sinks> section controls the log level of the client (freeorion.log), server
    (freeoriond.log) and AI (AI_x.log) files.

    The <sources> section controls the log level of the named loggers.

*/

// The logging levels.
enum class LogLevel {debug, info, warn, error, internal_logger};

constexpr LogLevel max_LogLevel = LogLevel::error;
constexpr LogLevel min_LogLevel = LogLevel::debug;

FO_COMMON_API std::string to_string(const LogLevel level);
FO_COMMON_API LogLevel to_LogLevel(const std::string& name);

// Prefix \p name to create a global logger name less likely to collide.
#define FO_GLOBAL_LOGGER_NAME(name) fo_logger_global_##name

/** Initializes the logging system. Log to the given file.  If the file already
 * exists it will be deleted. \p root_logger_name is the name by which the
 * default logger "" appears in the log file.*/
FO_COMMON_API void InitLoggingSystem(const std::string& logFile, const std::string& root_logger_name);

/** A type for loggers (sources) that allows for severity and a logger name (channel in
    boost parlance) and supports multithreading.*/
using NamedThreadedLogger = boost::log::sources::severity_channel_logger_mt<
    LogLevel,     ///< the type of the severity level
    std::string   ///< the channel name of the logger
    >;

// Lookup and/or register the \p name logger in OptionsDB.  Sets the initial level.
FO_COMMON_API void RegisterLoggerWithOptionsDB(NamedThreadedLogger& logger, const std::string& name);

// Place in source file to create the previously defined global logger \p name
#define CreateThreadedLogger(name)                                      \
    BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(                                \
        FO_GLOBAL_LOGGER_NAME(name), NamedThreadedLogger)               \
    {                                                                   \
        auto lg = NamedThreadedLogger(                                  \
            (boost::log::keywords::severity = LogLevel::debug),         \
            (boost::log::keywords::channel = #name));                   \
        RegisterLoggerWithOptionsDB(lg, #name);                         \
        return lg;                                                      \
    }

// Create the default logger
CreateThreadedLogger();

/** Sets the \p threshold of \p source.  \p source == "" is the default logger.*/
FO_COMMON_API void SetLoggerThreshold(const std::string& source, LogLevel threshold);

BOOST_LOG_ATTRIBUTE_KEYWORD(log_severity, "Severity", LogLevel);
BOOST_LOG_ATTRIBUTE_KEYWORD(log_channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_src_filename, "SrcFilename", std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(log_src_linenum, "SrcLinenum", int);

#define __BASE_FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define FO_LOGGER(name, lvl)                                            \
    BOOST_LOG_STREAM_WITH_PARAMS(                                       \
        FO_GLOBAL_LOGGER_NAME(name)::get(),                             \
        (boost::log::keywords::severity = lvl))                         \
    << boost::log::add_value("SrcFilename", __BASE_FILENAME__)          \
    << boost::log::add_value("SrcLinenum", __LINE__)

#define DebugLogger(name) FO_LOGGER(name, LogLevel::debug)

#define InfoLogger(name) FO_LOGGER(name, LogLevel::info)

#define WarnLogger(name) FO_LOGGER(name, LogLevel::warn)

#define ErrorLogger(name) FO_LOGGER(name, LogLevel::error)


extern int g_indent;

/** A function that returns the correct amount of spacing for the current
  * indentation level during a dump. */
std::string DumpIndent();

#endif // _Logger_h_
