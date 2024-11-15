#include <thread>
#include <climits>
#include <mutex>
#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <semaphore>
#include <sstream>

void usage (const char *);
int get_nthreads (const char *);
void routine (std::stringstream &, std::mutex &, std::size_t &, std::mutex &, 
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
    std::stringstream ss;
    if (!is) {
        std::cout << "Error opening file\n";
        return EXIT_FAILURE;
    }
    ss << is.rdbuf ();
    is.close ();
    std::mutex m_ss;

    /* Shared current word position variable and its mutex. */
    std::size_t word_position = (std::size_t)(-1);
    std::mutex m_word_position;

    /* Semaphore showing how many threads have finished but aren't joined yet. 
    */
    std::counting_semaphore s_joinable (0);


    /* Launch all threads. */
    std::list<std::thread> threads;
    for (int i = 0; i < nthreads; ++i) {
        threads.push_back (std::thread (routine, std::ref (ss), std::ref (m_ss), 
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

struct word {
    std::string word;
    std::size_t word_end;
};

const int WORDS_AT_ONCE = 512;
/* Read WORDS_AT_ONCE words from ss, store in result. Return 1 no words were 
   extracted, 0 otherwise. */
int read_words (std::stringstream &ss, std::mutex &m_ss, 
                std::array<word, WORDS_AT_ONCE> &result) {

    m_ss.lock();

    if (ss.eof()) {
        m_ss.unlock ();
        return 1;
    }

    /* Manually perform 1st iteration to check if there were words. */
    auto it = result.begin(), end = result.end();
    ss >> it->word;
    it->word_end = ss.tellg ();
    if (it->word.length() == 0) {
        m_ss.unlock ();
        return 1;
    }
    ++it;

    /* Loop for other words. */
    for (; it != end; ++it) {
        ss >> it->word;
        it->word_end = ss.tellg ();
    }

    m_ss.unlock ();
    return 0;
}
#if 0
struct word_parser {
    const size_t DEFAULT_STRSTRBUF_SIZE = 8192;
    std::stringstream ss_;
    std::ifstream &is_;
    std::mutex &m_is_;

    word_parser (std::ifstream &is, std::mutex &m_is):
        is_(is), m_is_(m_is)
    {}

    void refill_strstream () {
        if (!ss_.eof())
            return;
        ss_.
        char buf [DEFAULT_STRSTRBUF_SIZE];
    }

    word get_word () {
        refill_strstream ();
        ss_ << is_.rdbuf ();
    }
};
#endif

void routine (std::stringstream &ss, std::mutex &m_ss, std::size_t &word_position, 
              std::mutex &m_word_position, const std::string &target, 
              std::counting_semaphore<INT32_MAX> &s_joinable) {
    
    std::array <word, WORDS_AT_ONCE> words;
    for (;;) {
        
        /* Read next word. */
        if (read_words (ss, m_ss, words) == 1)
            goto done;

        for (auto it = words.begin(), end = words.end(); it != end; ++it) {
            if (it->word != target)
                continue;

            /* Answer is either this word's position or position of some other 
            word that another thread is processing now, so filestream can be 
            closed. */
            m_ss.lock();
            ss.str ({});
            m_ss.unlock ();

            /* Calculate where first byte of word was. */
            std::size_t word_begin = it->word_end - it->word.length();

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