#include <deque>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <semaphore.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#define key 1

using namespace std;

const int NUM_LEDS_WIDTH = 32;
const int NUM_LEDS_HEIGHT = 20;
const int NUM_LEDS_TOTAL = 2 * (NUM_LEDS_WIDTH + NUM_LEDS_HEIGHT);

const char SHM_NAME[] = "/shm_leds";
const char SEM_NAME[] = "/sem_leds";
const mode_t SHM_MODE = 0777;
const off_t SHM_SIZE = NUM_LEDS_TOTAL*3 + 2;

const int INTENSITY_MIN = 0;
const int INTENSITY_MAX = 100;

enum Key {
    ARROW_RIGHT,
    ARROW_LEFT
};

const map<deque<char>, Key> CHAR_SEQUENCES = {
    {{(char)27, (char)91, (char)67}, ARROW_RIGHT},
    {{(char)27, (char)91, (char)68}, ARROW_LEFT }
};

sem_t *sem = nullptr;
int shm_fd;
void *shm = nullptr;

int openShm(){
    sem = sem_open(SEM_NAME, O_CREAT, 0777, 1);
    if (sem == SEM_FAILED){
        perror("[INTENSITY] Could not open semaphore");
        return 1;
    }

    shm_fd = shm_open(SHM_NAME, O_RDWR, SHM_MODE);
    if(shm_fd == -1){
        perror("[INTENSITY] Could not open shm");
        return 1;
    }

    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm == MAP_FAILED){
        perror("[INTENSITY] Could not map shm");
        return 1;
    }

    return 0;
}

int closeShm(){
    if(munmap((void*)shm, SHM_SIZE) != 0) return 1;

    if(close(shm_fd) != 0) return 1;

    if(shm_unlink(SHM_NAME) != 0) return 1;

    if (sem_destroy(sem) != 0) return 1;

    if (sem_unlink(SEM_NAME) != 0) return 1;

    return 0;
}

void changeIntensity(int delta){
    sem_wait(sem);

    uint16_t *intensity = (uint16_t*)((uint8_t*)(shm)+NUM_LEDS_TOTAL*3);
    int intensity_int = *intensity;

    if(intensity_int + delta < INTENSITY_MIN){
        *intensity = INTENSITY_MIN;
    } else if(intensity_int + delta > INTENSITY_MAX){
        *intensity = INTENSITY_MAX;
    } else {
        *intensity += delta;
    }

    int intensity_final = *intensity;

    sem_post(sem);

    cout << " Changed intensity to " << intensity_final << endl;
}

void callbackSIGINT(int sig, siginfo_t *info, void *ucontext) {
    fprintf(stderr, "SIGINT callback\n");

    if(closeShm()) exit(1);

    exit(0);
}

int setupSIGINT(){
    struct sigaction act;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = callbackSIGINT;

    if(sigaction(SIGINT, &act, NULL) != 0){ perror("sigaction int"); return 1; }

    return 0;
}

int main(){
    if(openShm()){
        return 1;
    }

    if(setupSIGINT()){
        fprintf(stderr, "[INTENSITY] Could not setup SIGINT");
        return 1;
    }

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

    if(closeShm()){
        fprintf(stderr, "[INTENSITY] Could not close shm");
        return 1;
    }

    return 0;
}
