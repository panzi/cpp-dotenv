#include "dotenv.h"

static std::string fix_newlines(std::string_view src) {
    std::string buf;
    std::string::size_type index = 0;
    while (index < src.size()) {
        std::string::size_type cr_index = src.find('\r', index);
        if (cr_index == std::string::npos) {
            break;
        }
        buf.append(src.substr(index, cr_index));
        buf.append("\n");
        index = cr_index + 1;
        if (src.size() > index && src[index] == '\n') {
            // was \r\n, single \r otherwise
            ++ index;
        }
    }
    buf.append(src.substr(index));
    return buf;
}

static inline std::string::size_type skip_ws_inline(std::string_view src, std::string::size_type index) {
    index = src.find_first_not_of(" \t\x0B\x0C", index);
    if (index == std::string::npos) {
        return src.size();
    }
    return index;
}

static inline std::string::size_type skip_ws(std::string_view src, std::string::size_type index, std::string::size_type& line_start) {
    while (index < src.size()) {
        char ch = src[index];
        switch (ch) {
            case ' ':
            case '\t':
            case '\x0B':
            case '\x0C':
                break;
            case '\n':
                line_start = index + 1;
                break;
            default:
                return index;
        }
        ++ index;
    }
    return index;
}

static std::string::size_type skip_to_quote_end(std::string_view src, std::string::size_type index, char quote, std::string::size_type& line_start) {
    // In order to correctly emulate backtracking in the regular expressions
    // '(\\'|[^'])*' and "(\\"|[^""])*" we need to remember the last escaped
    // quote and use that if there is no unescaped quote in the rest of the
    // file.
    std::string::size_type new_line_start = line_start;
    std::string::size_type esc_line_start = new_line_start;
    std::string::size_type esc_index = std::string::npos;

    while (index < src.size()) {
        char ch = src[index];
        if (ch == '\\') {
            if (index + 1 < src.size() && src[index + 1] == quote) {
                ++ index;
                esc_line_start = new_line_start;
                esc_index = index;
            }
        } else if (ch == '\n') {
            new_line_start = index + 1;
        } else if (ch == quote) {
            line_start = new_line_start;
            return index;
        }
        ++ index;
    }

    if (esc_index != std::string::npos) {
        // SYNTAX ERROR: The last quote in the file is still escaped: \"
        // Recovering by using that as the end-quote, like the original does:
        line_start = esc_line_start;
    }

    return esc_index;
}

static inline bool is_vardef(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-';
}

static std::string::size_type find_vardef_end(std::string_view src, std::string::size_type index) {
    while (index < src.size()) {
        if (!is_vardef(src[index])) {
            break;
        }
        ++ index;
    }
    return index;
}

static std::string::size_type find_value_end(std::string_view src, std::string::size_type index) {
    while (index < src.size()) {
        char ch = src[index];
        if (ch == '#' || ch == '\n') {
            break;
        }
        ++ index;
    }
    return index;
}

static inline std::string::size_type find_line_end(std::string_view src, std::string::size_type index) {
    index = src.find('\n', index);
    if (index == std::string::npos) {
        return src.size();
    }
    return index;
}

static void replace_all(std::string& str, const std::string_view from, const std::string_view to) {
    std::string::size_type start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

static std::string unescape_double_quoted(std::string_view src) {
    std::string buf { src };

    replace_all(buf, "\\n", "\n");
    replace_all(buf, "\\r", "\r");

    return buf;
}

static void trim_space(std::string& src) {
    std::string::size_type index = skip_ws_inline(src, 0);
    if (index > 0) {
        src.erase(0, index - 1);
    }
    index = src.find_last_not_of(" \t\x0B\x0C");
    if (index != std::string::npos) {
        src.erase(index + 1);
    }
}

void dotenv::parse(std::string_view input, std::unordered_map<std::string, std::string>& env, bool overwrite) {
    std::string buf { fix_newlines(input) };
    std::string_view src { buf };

    std::string::size_type index = 0;
    std::string::size_type line_start = 0;

    while (index < src.size()) {
        index = skip_ws(src, index, line_start);

        if (index >= src.size())
            break;

        if (src[index] == '#') {
            index = find_line_end(src, index);
            continue;
        }

        std::string::size_type key_start = index;
        std::string::size_type key_end = find_vardef_end(src, index);
        std::string_view key_view { src.substr(key_start, key_end - key_start) };

        index = skip_ws(src, key_end, line_start);

        if (index < src.size() && is_vardef(src[index]) && key_view == "export") {
            key_start = index;
            key_end = find_vardef_end(src, index);
            key_view = src.substr(key_start, key_end - key_start);
            index = skip_ws(src, key_end, line_start);
        }

        if (key_start == key_end) {
            // SYNTAX ERROR: expected variable name, found unexpected character or EOF
            // trying to recover (alternatively throw exception or at least log an error?):
            index = find_line_end(src, index);
            continue;
        }

        std::string_view tail = src.substr(index);
        if (!tail.starts_with('=') && !tail.starts_with(':')) {
            // SYNTAX ERROR: expected '=', found unexpected character or EOF
            // trying to recover:
            if (key_end < line_start && index < src.size() && is_vardef(src[index])) {
                // We have failed to parse a variable definition where the `=` is on another line
                // to the variable name. So now we need to retry parsing from the start of this
                // new line in order to correctly emulate the regular expression.
            } else {
                index = find_line_end(src, index);
            }
            continue;
        }

        if (index != key_end && tail.starts_with(':')) {
            // SYNTAX ERROR: there cannot be space between the variable name and ':'
            // trying to recover:
            index = find_line_end(src, index);
            continue;
        }

        ++ index;
        index = skip_ws_inline(src, index);
        std::string::size_type value_start = index;
        std::string::size_type value_end = index;
        bool quoted = false;
        char ch = index < src.size() ? src[index] : 0;
        // simulate matching of the string literal regular expression
        if (ch == '"' || ch == '\'' || ch == '`') {
            std::string::size_type line_start_backup = line_start;
            std::string::size_type index_backup = index;

            ++ index;
            index = skip_to_quote_end(src, index, ch, line_start);
            if (index != std::string::npos) {
                quoted = true;
                value_end = index + 1;
                index = skip_ws_inline(src, value_end);

                char ch = index < src.size() ? src[index] : 0;
                if (ch == '#') {
                    index = find_line_end(src, index);
                } else if (index < src.size() && ch != '\n') {
                    // garbage after quoted string
                    // back out of parsing a quoted string and fallback to normal string
                    quoted = false;
                    line_start = line_start_backup;
                    index = index_backup;
                }
            } else {
                // no valid quoted string
                line_start = line_start_backup;
                index = index_backup;
            }
        }

        if (!quoted) {
            value_end = find_value_end(src, index);
            index = value_end;
        }

        std::string_view value_view { src.substr(value_start, value_end - value_start) };
        char quote = value_view.size() > 0 ? value_view[0] : 0;

        std::string value { value_view };
        if (value_view.size() > 1 && (quote == '"' || quote == '\'' || quote == '`') && value_view.ends_with(quote)) {
            value.pop_back();
            value.erase(0, 1);
        } else {
            trim_space(value);
        }

        if (quote == '"') {
            // yes, the original also applies unescape for a sorta
            // unquoted string that just starts with a double quote
            value = unescape_double_quoted(value);
        }

        index = skip_ws_inline(src, index);

        if (index < src.size()) {
            char ch = src[index];
            if (ch == '#') {
                index = find_line_end(src, index);
            } else if (ch != '\n') {
                // SYNTAX ERROR: expected end of line, end of file, or comment, but found something else
                // trying to recover:
                index = find_line_end(src, index);

                // Skip setting the already parsed environment variable, because
                // in the original the regular expression wouldn't have matched.
                continue;
            }
        }

        std::string key { key_view };
        if (overwrite || env.find(key) == env.end()) {
            env.insert_or_assign(key, value);
        }
    }
}
