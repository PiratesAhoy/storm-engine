#ifndef INIFILE_H
#define INIFILE_H

#include <string>
#include <map>
#include <vector>

class IniFile {
private:
    struct Line {
        enum Type { COMMENT, SECTION, KEY_VALUE, EMPTY };
        Type type;
        std::string content;
        std::string key;      // for KEY_VALUE lines
        std::string value;    // for KEY_VALUE lines
        std::string section;  // for KEY_VALUE lines
        std::string comment;  // inline comment if any
    };
    
    std::vector<Line> lines;
    std::string filename;
    bool isOpen;

public:
    IniFile();
    ~IniFile();
    
    // File operations
    bool open(const std::string& filename);
    bool save();
    bool saveAs(const std::string& filename);
    void close();
    
    // Read operations
    std::string getValue(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    bool hasKey(const std::string& section, const std::string& key) const;
    std::vector<std::string> getSections() const;
    std::vector<std::string> getKeys(const std::string& section) const;
    
    // Write operations
    void setValue(const std::string& section, const std::string& key, const std::string& value);
    void removeKey(const std::string& section, const std::string& key);
    void removeSection(const std::string& section);
    
private:
    void parseLine(const std::string& line, Line& parsedLine, std::string& currentSection);
    std::string trim(const std::string& str) const;
    int findKeyLine(const std::string& section, const std::string& key) const;
    int findSectionLine(const std::string& section) const;
};

#endif // INIFILE_H