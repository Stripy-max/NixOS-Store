// Pure C++ (no Qt) helpers: validation, ranking and output parsing.
// Kept free of Qt so they can be unit-tested anywhere (tests/test_logic.cpp).
#pragma once
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace stripy {

inline std::string lower(std::string s)
{
    for (auto &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline bool contains(const std::string &hay, const std::string &needle)
{
    return !needle.empty() && hay.find(needle) != std::string::npos;
}

inline bool startsWith(const std::string &s, const std::string &p)
{
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

// A nixpkgs attribute path such as "firefox" or "kdePackages.kdenlive".
// Strict on purpose: it ends up in a config file and in process arguments.
inline bool isValidAttr(const std::string &a)
{
    if (a.empty() || a.size() > 200) return false;
    bool segStart = true;
    for (char c : a) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (c == '.') {
            if (segStart) return false;           // empty segment / leading dot
            segStart = true;
            continue;
        }
        if (segStart) {
            if (!(std::isalnum(u) || c == '_')) return false;
            segStart = false;
        } else if (!(std::isalnum(u) || c == '_' || c == '-' || c == '\'' || c == '+')) {
            return false;
        }
    }
    return !segStart;                              // must not end with '.'
}

// Name of a nixosConfigurations.<host> entry.
inline bool isValidHost(const std::string &h)
{
    if (h.empty() || h.size() > 100) return false;
    for (size_t i = 0; i < h.size(); ++i) {
        const unsigned char u = static_cast<unsigned char>(h[i]);
        const bool ok = std::isalnum(u) || h[i] == '_' || (i > 0 && (h[i] == '-' || h[i] == '.'));
        if (!ok) return false;
    }
    return true;
}

// Absolute directory that is later embedded in "path:<dir>#<host>".
inline bool isSafeFlakeDir(const std::string &d)
{
    if (d.size() < 2 || d.size() > 4096 || d[0] != '/') return false;
    for (char c : d) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7f || c == '#' || c == '?' || c == '"') return false;
    }
    return true;
}

inline bool isHttpUrl(const std::string &u)
{
    const std::string l = lower(u);
    if (!(startsWith(l, "https://") || startsWith(l, "http://"))) return false;
    for (char c : u)
        if (static_cast<unsigned char>(c) < 0x21) return false;   // no spaces / control chars
    return u.size() < 2048;
}

// `nix search` takes a regex. Escape user input so it is searched literally and
// can never be interpreted as a command-line option.
inline std::string escapeRegex(const std::string &in)
{
    std::string out;
    static const std::string special = "\\.^$|?*+()[]{}";
    for (size_t i = 0; i < in.size(); ++i) {
        const char c = in[i];
        if (special.find(c) != std::string::npos || (i == 0 && c == '-')) out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

inline std::vector<std::string> splitWords(const std::string &s)
{
    std::istringstream is(s);
    std::vector<std::string> out;
    for (std::string w; is >> w;) out.push_back(lower(w));
    return out;
}

struct SearchFields {
    std::string name, attr, description;
    std::vector<std::string> keywords;
};

// 0 = no match. Every word of the query has to match something.
inline int matchScore(const std::string &query, const SearchFields &f)
{
    const auto terms = splitWords(query);
    if (terms.empty()) return 0;
    const std::string name = lower(f.name), attr = lower(f.attr), desc = lower(f.description);
    const std::string leaf = attr.substr(attr.rfind('.') == std::string::npos ? 0 : attr.rfind('.') + 1);
    int total = 0;
    for (const auto &t : terms) {
        int best = 0;
        if (name == t || leaf == t) best = 100;
        else if (startsWith(name, t) || startsWith(leaf, t)) best = 80;
        else if (contains(name, t)) best = 60;
        else if (contains(attr, t)) best = 50;
        for (const auto &k : f.keywords) {
            const std::string kl = lower(k);
            if (kl == t) best = std::max(best, 70);
            else if (startsWith(kl, t)) best = std::max(best, 55);
            else if (contains(kl, t)) best = std::max(best, 40);
        }
        if (best == 0 && contains(desc, t)) best = 20;
        if (best == 0) return 0;
        total += best;
    }
    return total;
}

// Maps a line of nixos-rebuild / nix output to a user-facing stage, but ONLY
// for lines that really indicate that stage. Empty string = nothing to report.
inline std::string stageForLine(const std::string &line)
{
    const std::string l = lower(line);
    if (contains(l, "building the system configuration")) return "Building the system configuration…";
    if (contains(l, "will be fetched")) return "Downloading packages…";
    if (contains(l, "will be built")) return "Building packages…";
    if (contains(l, "activating the configuration") || contains(l, "switching to system configuration"))
        return "Applying the new configuration…";
    if (contains(l, "evaluating") && contains(l, "flake")) return "Evaluating the configuration…";
    return std::string();
}

// "kdePackages.kdenlive" -> "Kdenlive", "protonup-qt" -> "Protonup Qt"
inline std::string humanName(const std::string &attr)
{
    std::string leaf = attr.substr(attr.rfind('.') == std::string::npos ? 0 : attr.rfind('.') + 1);
    bool cap = true;
    for (auto &c : leaf) {
        if (c == '-' || c == '_') { c = ' '; cap = true; continue; }
        if (cap) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        cap = false;
    }
    return leaf;
}

} // namespace stripy
