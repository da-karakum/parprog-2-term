#include <thread>
#include <climits>
#include <mutex>
#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <semaphore>

void usage (const char *);
int get_nthreads (const char *);
void routine (std::ifstream &, std::mutex &, std::size_t &, std::mutex &, 
              const std::string &, std::counting_semaphore<INT32_MAX> &);



int main (int argc, char *argv []) {
    if (argc != 4) {
        usage (argv[0]);
        exit (EXIT_FAILURE);
    }

    /* Number of threads. */
    int nthreads = get_nthreads (argv[1]);

    /* String being searched. */
    std::string target (argv[2]);

    /* Shared ifstream for file and its mutex. */
    std::ifstream is (argv[3]);
    std::mutex m_is;

    /* Shared current word position variable and its mutex. */
    std::size_t word_position = (std::size_t)(-1);
    std::mutex m_word_position;

    /* Semaphore showing how many threads have finished but aren't joined yet. 
    */
    std::counting_semaphore s_joinable (0);


    /* Launch all threads. */
    std::list<std::thread> threads;
    for (int i = 0; i < nthreads; ++i) {
        threads.push_back (std::thread (routine, std::ref (is), std::ref (m_is), 
                        std::ref (word_position), std::ref (m_word_position), 
                        std::ref (target), std::ref (s_joinable)));
    }

    while (!threads.empty()) {
        /* Wait for the next thread to release semaphore and busywait for him 
           returning. */
        s_joinable.acquire();
        auto it = threads.begin();
        while (true) {
            if (it->joinable()) {
                it->join ();
                threads.erase (it++);
                break;
            } else {
                ++it;
            }
            if (it == threads.end())
                it = threads.begin();
        }
    }

    if (word_position == (std::size_t)(-1)) {
        std::cout << "Word not found\n";
    } else {
        std::cout << word_position << "\n";
    }

    return EXIT_SUCCESS;
}

void routine (std::ifstream &is, std::mutex &m_is, std::size_t &word_position, 
              std::mutex &m_word_position, const std::string &target, 
              std::counting_semaphore<INT32_MAX> &s_joinable) {

    for (int step = 0;;step++) {
        /*if (step & 0xF == 0) {
            m_word_position.lock();
            std::size_t word_position_copy = word_position;
            m_word_position.unlock();
            if (word_position_copy != -1)
                goto done;
        }*/
        std::string word;

        /* Read next word. */
        m_is.lock();
        is >> word;
        std::size_t word_end = is.tellg ();
        m_is.unlock ();

        /* If word is emptystring, we reached eof or some spaces at the end of 
           file. */
        if (word.length() == 0) {
            goto done;
        }

        if (word == target) {
            /* Answer is either this word's position or position of some other 
            word that another thread is processing now, so filestream can be 
            closed. */
            m_is.lock();
            is.seekg (0, is.end);
            m_is.unlock ();

            /* Calculate where first byte of word was. */
            std::size_t word_begin = word_end - word.length();

            /* Store an answer if better one wasn't found. */
            m_word_position.lock ();
            word_position = std::min (word_position, word_begin);
            m_word_position.unlock ();

            goto done;
        }
    
    }
    done:
    s_joinable.release();
    return;
}




int get_nthreads (const char *nthreads_str) {
    char *end;
    unsigned long res = strtoul (nthreads_str, &end, 10);
    errno = 0;
    if (*end != '\0' || errno == ERANGE || res > INT32_MAX) {
        printf ("Incorrect nthreads\n");
        exit (EXIT_FAILURE);
    }
    return res;
}

void usage (const char *argv0) {
    printf ("Usage: %s <num_threads> <word> <file>\n", argv0);
}