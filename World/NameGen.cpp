#include "NameGen.h"
#include <algorithm>   // std::sort, std::min, std::max
#include <cctype>      // toupper, tolower
#include <vector>

using namespace std;

// Returns a lowercase copy of the text. ('text' is a COPY, so the caller's string is untouched.)
static string Lower(string text)
{
    for (char& ch : text)                 // 'char&' = the real character, so the change sticks
    {
        ch = (char)tolower((unsigned char)ch);
    }
    return text;
}

string MakeName(const Culture& culture, bool forPerson, Rng& rng)
{
    const NameParts& parts = forPerson ? culture.people : culture.settlement;

    const int minLength = 4;
    const int maxLength = forPerson ? 9 : 12;      // people get shorter names than places

    string name;
    for (int attempt = 0; attempt < 100; attempt++)
    {
        // pick a start and an ending and join them
        const string& start  = parts.starts[rng.range(0, (int)parts.starts.size() - 1)];
        const string& ending = parts.ends[rng.range(0, (int)parts.ends.size() - 1)];
        name = start + ending;

        // too short or too long? try again
        if ((int)name.length() < minLength || (int)name.length() > maxLength)
        {
            continue;
        }

        // the same word twice, like "Mossmoss"? try again (compare without capitals)
        if (Lower(start) == Lower(ending))
        {
            continue;
        }

        name[0] = (char)toupper((unsigned char)name[0]);
        return name;
    }

    // 100 bad tries in a row is very unlikely; return the last one rather than nothing
    name[0] = (char)toupper((unsigned char)name[0]);
    return name;
}

const Culture* CultureForTile(int tileX, int tileY, int worldWidth)
{
    (void)tileY;                                    // unused for now; Step 2 (nations) will use it

    // Collect the culture ids and sort them. An unordered_map has NO guaranteed order, so without
    // sorting the same seed could give different names on different computers.
    vector<string> ids;
    for (const auto& pair : gameData.cultures)
    {
        ids.push_back(pair.first);
    }
    if (ids.empty() || worldWidth <= 0)
    {
        return nullptr;                             // no data loaded: the callers fall back ("Stranger" / the old village names)
    }
    sort(ids.begin(), ids.end());

    // 0 at the left edge ... last culture at the right edge
    int band = (int)((long long)tileX * (int)ids.size() / worldWidth);
    band = max(0, min(band, (int)ids.size() - 1));

    return FindCulture(gameData, ids[band]);
}