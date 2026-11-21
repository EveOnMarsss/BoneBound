#include <iostream>
#include <string>
#include "../Systems/DataLoader.h"
#include "../World/NameGen.h"

using namespace std;

int main()
{
    LoadGameData(gameData);

    Rng rng(1);
    //for loop to loop gamedata.cultures then print inside one culture print 20 names for settlements and 20 names for people then do next culture
    for (const auto& culturePair : gameData.cultures)
    {
        //get the culture from the pair
        const Culture& culture = culturePair.second;
        //display the culture id
        cout << "Culture: " << culture.id << endl;
        //display the settlements name
        cout << "Settlement Names:" << endl;
        //loop 20 times to generate settlement names
        for (int i = 0; i < 20; ++i)
        {
            string settlementName = MakeName(culture, false, rng);
            cout << settlementName << endl;
        }
        //display the people name
        cout << "People Names:" << endl;
        //loop 20 times to generate people names
        for (int i = 0; i < 20; ++i)
        {
            string peopleName = MakeName(culture, true, rng);
            cout << peopleName << endl;
        }
        //display a line to separate cultures
        cout << "------------------------" << endl;
    }
    cout << "Press Enter to generate names again, or Ctrl+C to exit..." << endl;

    return 0;
}