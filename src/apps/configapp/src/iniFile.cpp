#include "IniFile.h"
#include <algorithm>
#include <fstream>
#include <sstream>

IniFile::IniFile()
    : isOpen(false)
{
}

IniFile::~IniFile()
{
    close();
}

bool IniFile::open(const std::string &filename)
{
    this->filename = filename;
    lines.clear();

    std::ifstream file(filename);
    if (!file.is_open())
    {
        isOpen = false;
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        Line parsedLine;
        parseLine(line, parsedLine, currentSection);
        lines.push_back(parsedLine);
    }

    file.close();
    isOpen = true;
    return true;
}

bool IniFile::save()
{
    if (!isOpen || filename.empty())
    {
        return false;
    }

    return saveAs(filename);
}

bool IniFile::saveAs(const std::string &filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    for (const auto &line : lines)
    {
        file << line.content << std::endl;
    }

    file.close();
    return true;
}

void IniFile::close()
{
    lines.clear();
    filename.clear();
    isOpen = false;
}

std::string IniFile::getValue(const std::string &section, const std::string &key, const std::string &defaultValue) const
{
    int lineIndex = findKeyLine(section, key);
    if (lineIndex >= 0)
    {
        return lines[lineIndex].value;
    }
    return defaultValue;
}

bool IniFile::hasKey(const std::string &section, const std::string &key) const
{
    return findKeyLine(section, key) >= 0;
}

std::vector<std::string> IniFile::getSections() const
{
    std::vector<std::string> sections;
    for (const auto &line : lines)
    {
        if (line.type == Line::SECTION)
        {
            std::string sectionName = line.content.substr(1, line.content.length() - 2);
            sections.push_back(trim(sectionName));
        }
    }
    return sections;
}

std::vector<std::string> IniFile::getKeys(const std::string &section) const
{
    std::vector<std::string> keys;
    for (const auto &line : lines)
    {
        if (line.type == Line::KEY_VALUE && line.section == section)
        {
            keys.push_back(line.key);
        }
    }
    return keys;
}

void IniFile::setValue(const std::string &section, const std::string &key, const std::string &value)
{
    int lineIndex = findKeyLine(section, key);

    if (lineIndex >= 0)
    {
        // Key exists, update it while preserving comment and spacing
        Line &line = lines[lineIndex];
        line.value = value;

        // Reconstruct the content
        std::string newContent = line.key + " = " + value;
        if (!line.comment.empty())
        {
            newContent += line.commentPrefix + line.comment;
        }
        line.content = newContent;
    }
    else
    {
        // Key doesn't exist, need to add it
        int sectionIndex = findSectionLine(section);

        if (sectionIndex < 0 && !section.empty())
        {
            // Section doesn't exist, create it
            // Add a blank line before the section if there are existing lines
            if (!lines.empty() && lines.back().type != Line::EMPTY)
            {
                Line blankLine;
                blankLine.type = Line::EMPTY;
                blankLine.content = "";
                lines.push_back(blankLine);
            }

            Line sectionLine;
            sectionLine.type = Line::SECTION;
            sectionLine.content = "[" + section + "]";
            lines.push_back(sectionLine);
            sectionIndex = lines.size() - 1;
        }

        // Create new key-value line
        Line newLine;
        newLine.type = Line::KEY_VALUE;
        newLine.key = key;
        newLine.value = value;
        newLine.section = section;
        newLine.content = key + " = " + value;
        newLine.commentPrefix.clear();
        newLine.comment.clear();

        if (sectionIndex >= 0)
        {
            // Insert after the section or after the last key in this section
            int insertPos = sectionIndex + 1;
            while (insertPos < static_cast<int>(lines.size()) && (lines[insertPos].type != Line::SECTION) &&
                   (lines[insertPos].section == section || lines[insertPos].type != Line::KEY_VALUE))
            {
                insertPos++;
            }

            lines.insert(lines.begin() + insertPos, newLine);

            // If there's a section after this insertion, add a blank line after the new key
            if (insertPos + 1 < static_cast<int>(lines.size()) && lines[insertPos + 1].type == Line::SECTION)
            {
                Line blankLine;
                blankLine.type = Line::EMPTY;
                blankLine.content = "";
                lines.insert(lines.begin() + insertPos + 1, blankLine);
            }
        }
        else
        {
            // No section (global), add at the beginning
            int insertPos = 0;
            while (insertPos < static_cast<int>(lines.size()) && lines[insertPos].section.empty() &&
                   lines[insertPos].type != Line::SECTION)
            {
                insertPos++;
            }
            lines.insert(lines.begin() + insertPos, newLine);
        }
    }
}

void IniFile::removeKey(const std::string &section, const std::string &key)
{
    int lineIndex = findKeyLine(section, key);
    if (lineIndex >= 0)
    {
        lines.erase(lines.begin() + lineIndex);
    }
}

void IniFile::removeSection(const std::string &section)
{
    lines.erase(std::remove_if(lines.begin(), lines.end(),
                               [&section](const Line &line) {
                                   return (line.type == Line::SECTION && line.content == "[" + section + "]") ||
                                          (line.type == Line::KEY_VALUE && line.section == section);
                               }),
                lines.end());
}

void IniFile::parseLine(const std::string &line, Line &parsedLine, std::string &currentSection)
{
    std::string trimmedLine = trim(line);
    parsedLine.content = line;

    if (trimmedLine.empty())
    {
        parsedLine.type = Line::EMPTY;
    }
    else if (trimmedLine[0] == ';')
    {
        parsedLine.type = Line::COMMENT;
    }
    else if (trimmedLine[0] == '[')
    {
        // Look for section with possible inline comment
        size_t closeBracket = trimmedLine.find(']');
        if (closeBracket != std::string::npos)
        {
            parsedLine.type = Line::SECTION;
            currentSection = trimmedLine.substr(1, closeBracket - 1);
            currentSection = trim(currentSection);
        }
        else
        {
            parsedLine.type = Line::COMMENT; // Treat as comment if malformed
        }
    }
    else
    {
        // Look for key=value pattern
        size_t equalPos = trimmedLine.find('=');
        if (equalPos != std::string::npos)
        {
            parsedLine.type = Line::KEY_VALUE;
            parsedLine.section = currentSection;

            std::string keyPart = trim(trimmedLine.substr(0, equalPos));
            std::string valuePart = trimmedLine.substr(equalPos + 1);

            // Check for inline comment
            size_t commentPos = valuePart.find(';');
            if (commentPos != std::string::npos)
            {
                // Find the prefix (spaces/tabs) before the comment
                size_t prefixStart = commentPos;
                while (prefixStart > 0 && (valuePart[prefixStart - 1] == ' ' || valuePart[prefixStart - 1] == '\t'))
                    --prefixStart;
                parsedLine.commentPrefix = valuePart.substr(prefixStart, commentPos - prefixStart);
                parsedLine.comment = valuePart.substr(commentPos);
                valuePart = valuePart.substr(0, prefixStart);
            }
            else
            {
                parsedLine.commentPrefix.clear();
                parsedLine.comment.clear();
            }

            parsedLine.key = keyPart;
            parsedLine.value = trim(valuePart);
        }
        else
        {
            parsedLine.type = Line::COMMENT; // Treat as comment if no '=' found
        }
    }
}

std::string IniFile::trim(const std::string &str) const
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

int IniFile::findKeyLine(const std::string &section, const std::string &key) const
{
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        const Line &line = lines[i];
        if (line.type == Line::KEY_VALUE && line.section == section && line.key == key)
        {
            return i;
        }
    }
    return -1;
}

int IniFile::findSectionLine(const std::string &section) const
{
    for (int i = 0; i < static_cast<int>(lines.size()); i++)
    {
        if (lines[i].type == Line::SECTION)
        {
            // Extract section name from the line content
            const std::string &content = lines[i].content;
            std::string trimmedContent = trim(content);
            if (trimmedContent[0] == '[')
            {
                size_t closeBracket = trimmedContent.find(']');
                if (closeBracket != std::string::npos)
                {
                    std::string sectionName = trimmedContent.substr(1, closeBracket - 1);
                    sectionName = trim(sectionName);
                    if (sectionName == section)
                    {
                        return i;
                    }
                }
            }
        }
    }
    return -1;
}