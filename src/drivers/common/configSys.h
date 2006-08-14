#ifndef __CONFIGSYS_H
#define __CONFIGSYS_H

#include <map>
#include <string>

class Config {
private:
    std::string _dir;

    std::map<std::string, std::string>    _strOptMap;
    std::map<std::string, int>            _intOptMap;
    std::map<std::string, void (*)(void)> _fnOptMap;

    std::map<char, std::string>        _shortArgMap;
    std::map<std::string, std::string> _longArgMap;

private:
    int _addOption(char, const std::string &, const std::string &, int);
    int _load(void);
    int _parseArgs(int, char **);

public:
    const static int STRING   = 1;
    const static int INTEGER  = 2;
    const static int FUNCTION = 3;

public:
    Config(std::string d) : _dir(d) { }
    ~Config() { }

    /**
     * Adds a configuration option.  All options must be added before
     * parse().
     */
    int addOption(char, const std::string &,
                  const std::string &, int);
    int addOption(char, const std::string &,
                  const std::string &, const std::string &);
    int addOption(char, const std::string &,
                  const std::string &, void (*)(void));

    /**
     * Sets a configuration option.  Can be called at any time.
     */
    int setOption(const std::string &, int);
    int setOption(const std::string &, const std::string &);
    int setOption(const std::string &, void (*)(void));

    int getOption(const std::string &, std::string *);
    int getOption(const std::string &, const char **);
    int getOption(const std::string &, int *);

    /**
     * Parse the arguments.  Also read in the configuration file and
     * set the variables accordingly.
     */
    int parse(int, char **);

    /**
     * Save all of the current configuration options to the
     * configuration file.
     */
    int save();
};

#endif // !__CONFIGSYS_H
