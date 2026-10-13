#include "../src/logic.h"
#include <cstdio>
#include <cstdlib>
using namespace stripy;
static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)

int main()
{
    // attributes
    CHECK(isValidAttr("firefox"));
    CHECK(isValidAttr("kdePackages.kdenlive"));
    CHECK(isValidAttr("python3Packages.numpy"));
    CHECK(isValidAttr("libsigc++"));
    CHECK(isValidAttr("7zz"));
    CHECK(!isValidAttr(""));
    CHECK(!isValidAttr(".firefox"));
    CHECK(!isValidAttr("firefox."));
    CHECK(!isValidAttr("a..b"));
    CHECK(!isValidAttr("-rf"));
    CHECK(!isValidAttr("a b"));
    CHECK(!isValidAttr("a;b"));
    CHECK(!isValidAttr("$(reboot)"));
    CHECK(!isValidAttr("a\"b"));
    CHECK(!isValidAttr("a/b"));
    CHECK(!isValidAttr(std::string(201, 'a')));
    // host / dir
    CHECK(isValidHost("stripyos"));
    CHECK(isValidHost("my-laptop.local"));
    CHECK(!isValidHost("-x"));
    CHECK(!isValidHost("a#b"));
    CHECK(!isValidHost("a b"));
    CHECK(isSafeFlakeDir("/home/alice/stripyos"));
    CHECK(!isSafeFlakeDir("relative/dir"));
    CHECK(!isSafeFlakeDir("/home/a#b"));
    CHECK(!isSafeFlakeDir("/home/a?b"));
    CHECK(!isSafeFlakeDir("/home/a\nb"));
    CHECK(!isSafeFlakeDir("/"));
    // urls
    CHECK(isHttpUrl("https://www.mozilla.org/firefox/"));
    CHECK(!isHttpUrl("file:///etc/passwd"));
    CHECK(!isHttpUrl("javascript:alert(1)"));
    CHECK(!isHttpUrl("https://a b"));
    // regex escaping
    CHECK(escapeRegex("c++") == "c\\+\\+");
    CHECK(escapeRegex("-v") == "\\-v");
    CHECK(escapeRegex("a-b") == "a-b");
    CHECK(escapeRegex("(x)") == "\\(x\\)");
    // ranking
    SearchFields ff{"Firefox", "firefox", "Fast, private web browser", {"browser", "mozilla"}};
    SearchFields ch{"Chromium", "chromium", "Open-source web browser", {"browser"}};
    SearchFields kd{"Kdenlive", "kdePackages.kdenlive", "Video editor", {"video"}};
    CHECK(matchScore("firefox", ff) > matchScore("fire", ff));
    CHECK(matchScore("FIRE", ff) > 0);
    CHECK(matchScore("browser", ff) > 0 && matchScore("browser", ch) > 0);
    CHECK(matchScore("moz", ff) > 0);
    CHECK(matchScore("moz", ch) == 0);
    CHECK(matchScore("kdenlive", kd) >= 100);              // leaf of nested attr
    CHECK(matchScore("kdePackages", kd) > 0);              // full attr path
    CHECK(matchScore("video editor", kd) > 0);
    CHECK(matchScore("video zzz", kd) == 0);               // every word must match
    CHECK(matchScore("fast", ff) > 0);                     // description
    CHECK(matchScore("   ", ff) == 0);
    // stages: only real ones
    CHECK(stageForLine("building the system configuration...") == "Building the system configuration…");
    CHECK(stageForLine("these 4 derivations will be built:") == "Building packages…");
    CHECK(stageForLine("these 9 paths will be fetched (20 MiB download)") == "Downloading packages…");
    CHECK(stageForLine("activating the configuration...") == "Applying the new configuration…");
    CHECK(stageForLine("some random output").empty());
    // names
    CHECK(humanName("kdePackages.kdenlive") == "Kdenlive");
    CHECK(humanName("protonup-qt") == "Protonup Qt");

    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("all logic tests passed\n");
    return 0;
}
