#include <termios.h>
#include <iostream>
#include <deque>
#include <map>
#include <vector>

#define key 1

using namespace std;

enum Key {
    ARROW_RIGHT,
    ARROW_LEFT
};

const map<deque<char>, Key> CHAR_SEQUENCES = {
    {{(char)27, (char)91, (char)67}, ARROW_RIGHT},
    {{(char)27, (char)91, (char)68}, ARROW_LEFT }
};

void changeIntensity(int delta){
    cout << "delta=" << delta << endl;
}

int main(){
    struct termios term;
    tcgetattr(key, &term);
    term.c_lflag &= ~ICANON;
    tcsetattr(key, TCSANOW, &term);

    deque<char> buf;
    char c;
    while(cin >> c){
        buf.push_back(c);
        if(CHAR_SEQUENCES.find(buf) != CHAR_SEQUENCES.end()){
            switch(CHAR_SEQUENCES.at(buf)){
                case ARROW_RIGHT: changeIntensity(+1); break;
                case ARROW_LEFT : changeIntensity(-1); break;
                default: throw logic_error("No other value is allowed for enum Key");
            }
            buf.clear();
        }
    }

    return 0;
}
