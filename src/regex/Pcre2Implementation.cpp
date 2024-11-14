#include "Pcre2Implementation.h"
#include <cstring>
#include <format>
#include <pcre2.h>
namespace regex {
    class Pcre2Implementation::Pcre2Private {
    public:
        static std::expected<Pcre2Private *, std::string> generate(const std::string& pattern) {
            pcre2_compile_context *cc = pcre2_compile_context_create(nullptr);
            pcre2_set_newline(cc, PCRE2_NEWLINE_LF);
            uint8_t *table = (uint8_t *)pcre2_maketables(nullptr);
            table[842] = 0;                                   // 换行符不作为\s
            pcre2_set_character_tables(cc, table);
            // pcre2_set_compile_extra_options(cc, PCRE2_EXTRA_MATCH_LINE);
            int errornumber;
            PCRE2_SIZE erroroffset;
            pcre2_code *re = pcre2_compile(
                reinterpret_cast<PCRE2_SPTR>(pattern.data()), /* the pattern */
                PCRE2_ZERO_TERMINATED,                        /* indicates pattern is zero-terminated */
                PCRE2_UTF | PCRE2_MULTILINE,                  /* default options */
                &errornumber,                                 /* for error number */
                &erroroffset,                                 /* for error offset */
                cc);                                          /* use default compile context */
            if (re == NULL) {
                PCRE2_UCHAR buffer[256];
                pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
                return std::unexpected(std::format("PCRE2 compilation failed at offset {}: {}", erroroffset, (char *)buffer));
            }

            pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

            uint32_t nameCount;
            (void)pcre2_pattern_info(
                re,                                           /* the compiled pattern */
                PCRE2_INFO_NAMECOUNT,                         /* get the number of named substrings */
                &nameCount);                                  /* where to put the answer */

            /* Before we can access the substrings, we must extract the table for translating names to numbers, and the size of each entry in the table. */
            PCRE2_SPTR nameTable;
            (void)pcre2_pattern_info(
                re,                                           /* the compiled pattern */
                PCRE2_INFO_NAMETABLE,                         /* address of the table */
                &nameTable);                                  /* where to put the answer */

            uint32_t nameEntrySize;
            (void)pcre2_pattern_info(
                re,                                           /* the compiled pattern */
                PCRE2_INFO_NAMEENTRYSIZE,                     /* size of each entry in the table */
                &nameEntrySize);                              /* where to put the answer */
            return new Pcre2Private(cc, table, re, match_data, nameCount, nameTable, nameEntrySize);
        };

        std::expected<bool, std::string> exist(const std::string& subject, PCRE2_SIZE start) {
            /* Now run the match. */
            int rc = pcre2_match(
                re,                                           /* the compiled pattern */
                reinterpret_cast<PCRE2_SPTR>(subject.data()), /* the subject string */
                subject.length(),                             /* the length of the subject */
                start,                                        /* start at offset 0 in the subject */
                0,                                            /* default options */
                match_data,                                   /* block for storing the result */
                NULL                                          /* use default match context */
            );
            /* Matching failed: handle error cases */
            if (rc < 0) {
                switch (rc) {
                    case PCRE2_ERROR_NOMATCH: {
                        return false;
                    }
                }
                pcre2_get_error_message(rc, buffer.get(), maxBufferSize);
                return std::unexpected(std::format("PCRE2 matching failed: {}", (char *)buffer.get()));
            }
            return true;
        }

        std::expected<std::vector<FindInfo>, std::string> find(const std::string& subject, int times, PCRE2_SIZE start) {
            std::vector<FindInfo> findInfoList;
            FindInfo findInfo;
            /* Now run the match. */
            int rc = pcre2_match(
                re,                                                                    /* the compiled pattern */
                reinterpret_cast<PCRE2_SPTR>(subject.data()),                          /* the subject string */
                subject.length(),                                                      /* the length of the subject */
                start,                                                                 /* start at offset 0 in the subject */
                0,                                                                     /* default options */
                match_data,                                                            /* block for storing the result */
                NULL                                                                   /* use default match context */
            );
            /* Matching failed: handle error cases */
            if (rc < 0) {
                switch (rc) {
                    case PCRE2_ERROR_NOMATCH: {
                        return findInfoList;
                    }
                }
                pcre2_get_error_message(rc, buffer.get(), maxBufferSize);
                return std::unexpected(std::format("PCRE2 matching failed: {}", (char *)buffer.get()));
            }

            for (int i = 0; i < rc; i++) {
                findInfo.group.push_back({
                    ovector[2 * i],
                    ovector[2 * i + 1],
                    subject.substr(ovector[2 * i], ovector[2 * i + 1] - ovector[2 * i])
                });
            }

            if (nameCount > 0) {
                /* Now we can scan the table and, for each entry, print the number, the name,
                   and the substring itself. In the 8-bit library the number is held in two
                   bytes, most significant first. */

                PCRE2_SPTR tabptr = nameTable;
                for (uint32_t i = 0; i < nameCount; i++) {
                    size_t n = ((size_t)(tabptr[0]) << 8) | tabptr[1];
                    findInfo.namedGroup.emplace(std::string(reinterpret_cast<const char *>(tabptr + 2)), n);
                    tabptr += nameEntrySize;
                }
            }
            findInfoList.push_back(findInfo);
            for (int nowTimes = 1; nowTimes != times; ) {
                uint32_t options = 0; /* Normally no options */
                start = ovector[1];   /* Start at end of previous match */
                /* If the previous match was for an empty string, we are finished if we are
                   at the end of the subject. Otherwise, arrange to run another match at the
                   same point to see if a non-empty match can be found. */

                if (ovector[0] == ovector[1]) {
                    if (ovector[0] == subject.length()) {
                        break;
                    }
                    options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
                }
                /* Run the next matching operation */

                rc = pcre2_match(
                    re,                                           /* the compiled pattern */
                    reinterpret_cast<PCRE2_SPTR>(subject.data()), /* the subject string */
                    subject.length(),                             /* the length of the subject */
                    start,                                        /* starting offset in the subject */
                    options,                                      /* options */
                    match_data,                                   /* block for storing the result */
                    NULL);                                        /* use default match context */

                /* This time, a result of NOMATCH isn't an error. If the value in "options"
                   is zero, it just means we have found all possible matches, so the loop ends.
                   Otherwise, it means we have failed to find a non-empty-string match at a
                   point where there was a previous empty-string match. In this case, we do what
                   Perl does: advance the matching position by one character, and continue. We
                   do this by setting the "end of previous match" offset, because that is picked
                   up at the top of the loop as the point at which to start again.

                   There are two complications: (a) When CRLF is a valid newline sequence, and
                   the current position is just before it, advance by an extra byte. (b)
                   Otherwise we must ensure that we skip an entire UTF character if we are in
                   UTF mode. */
                if (rc == PCRE2_ERROR_NOMATCH) {
                    if (options == 0) {
                        break;                                    /* All matches found */
                    }
                    ovector[1] = start + 1;                       /* Advance one code unit */
                    if (start < subject.length() - 1 &&           /* we are at CRLF, */
                        subject[start] == '\r' &&
                        subject[start + 1] == '\n') {
                        ovector[1] += 1;                          /* Advance by one more. */
                    } else {                                      /* Otherwise, ensure we */
                        /* advance a whole UTF-8 */
                        while (ovector[1] < subject.length()) {   /* character. */
                            if ((subject[ovector[1]] & 0xc0) != 0x80) {
                                break;
                            }
                            ovector[1] += 1;
                        }
                    }
                    continue;                                     /* Go round the loop again */
                }
                /* Other matching errors are not recoverable. */

                if (rc < 0) {
                    pcre2_get_error_message(rc, buffer.get(), maxBufferSize);
                    return std::unexpected(std::format("Matching error {}", (char *)buffer.get()));
                }
                findInfo.group.clear();
                findInfo.namedGroup.clear();
                for (int i = 0; i < rc; i++) {
                    findInfo.group.push_back({
                        ovector[2 * i],
                        ovector[2 * i + 1],
                        subject.substr(ovector[2 * i], ovector[2 * i + 1] - ovector[2 * i])
                    });
                }

                if (nameCount > 0) {
                    /* Now we can scan the table and, for each entry, print the number, the name,
                       and the substring itself. In the 8-bit library the number is held in two
                       bytes, most significant first. */

                    PCRE2_SPTR tabptr = nameTable;
                    for (uint32_t i = 0; i < nameCount; i++) {
                        size_t n = ((size_t)(tabptr[0]) << 8) | tabptr[1];
                        findInfo.namedGroup.emplace(std::string(reinterpret_cast<const char *>(tabptr + 2)), n);
                        tabptr += nameEntrySize;
                    }
                }
                findInfoList.push_back(findInfo);
                nowTimes++;
            }
            return findInfoList;
        }

        std::expected<std::string, std::string> replace(std::string subject, const std::string& replacementText, int times, size_t start) {
            // 启用扩展替换处理，将未知的命名捕获组视为未设置，简单的未设置插入等于空字符串
            uint32_t defaultOptions = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY;
            if (times == 0) {
                defaultOptions = defaultOptions | PCRE2_SUBSTITUTE_GLOBAL;
                times = 1;
            }
            uint32_t options = defaultOptions;
            while (times--) {
                PCRE2_SIZE bufferSize = maxBufferSize - 1;
                size_t subjectLength = subject.length();
                int rc = pcre2_substitute(re,                              // 编译后的正则表达式
                    reinterpret_cast<PCRE2_SPTR>(subject.data()),          // 要进行替换的原始字符串
                    subjectLength,                                         // 原始字符串的长度
                    start,                                                 // 开始替换的位置
                    options,                                               // 替换选项
                    match_data,                                            // 匹配数据块，这里传递已构造数据库但不使用
                    nullptr,                                               // 匹配上下文，这里为nullptr表示不使用
                    reinterpret_cast<PCRE2_SPTR>(replacementText.data()),  // 替换文本
                    replacementText.length(),                              // 替换文本的长度
                    buffer.get(),                                          // 输出缓冲区，用于存放替换后的字符串
                    &bufferSize                                            // 输出缓冲区大小
                );
                /* Matching failed: handle error cases */
                if (rc <= 0) {
                    if (rc == PCRE2_ERROR_NOMATCH || rc == 0) {
                        if (options == defaultOptions) {
                            break;
                        }
                        options = defaultOptions;
                        ovector[1] = start + 1;                     /* Advance one code unit */
                        if (start < subject.length() - 1 &&         /* we are at CRLF, */
                            subject[start] == '\r' &&
                            subject[start + 1] == '\n') {
                            ovector[1] += 1;                        /* Advance by one more. */
                        } else {                                    /* Otherwise, ensure we */
                            /* advance a whole UTF-8 */
                            while (ovector[1] < subject.length()) { /* character. */
                                if ((subject[ovector[1]] & 0xc0) != 0x80) {
                                    break;
                                }
                                ovector[1] += 1;
                            }
                        }
                        start = ovector[1];
                        times++;
                        continue;
                    }
                    switch (rc) {
                        case PCRE2_ERROR_NOMEMORY: {
                            break;
                        }
                        default: {
                            pcre2_get_error_message(rc, buffer.get(), maxBufferSize);
                            return std::unexpected(std::format("PCRE2 matching failed: {}", (char *)buffer.get()));
                        }
                    }
                }
                if (bufferSize >= maxBufferSize) {
                    maxBufferSize = bufferSize + 1;
                    buffer = std::make_unique<PCRE2_UCHAR[]>(maxBufferSize);
                    rc = pcre2_substitute(re,                                 // 编译后的正则表达式
                        reinterpret_cast<PCRE2_SPTR>(subject.data()),         // 要进行替换的原始字符串
                        subjectLength,                                        // 原始字符串的长度
                        start,                                                // 开始替换的位置
                        options,                                              // 替换选项
                        nullptr,                                              // 匹配数据块，这里为nullptr表示不使用
                        nullptr,                                              // 匹配上下文，这里为nullptr表示不使用
                        reinterpret_cast<PCRE2_SPTR>(replacementText.data()), // 替换文本
                        replacementText.length(),                             // 替换文本的长度
                        buffer.get(),                                         // 输出缓冲区，用于存放替换后的字符串
                        &bufferSize                                           // 输出缓冲区大小
                    );
                    /* Matching failed: handle error cases */
                    if (rc < 0) {
                        switch (rc) {
                            case PCRE2_ERROR_NOMATCH: {
                                break;
                            }
                            default: {
                                pcre2_get_error_message(rc, buffer.get(), maxBufferSize);
                                return std::unexpected(std::format("PCRE2 matching failed: {}", (char *)buffer.get()));
                            }
                        }
                    }
                }
                options = defaultOptions;
                subject = reinterpret_cast<char *>(buffer.get());
                /* If the previous match was for an empty string, we are finished if we are
                   at the end of the subject. Otherwise, arrange to run another match at the
                   same point to see if a non-empty match can be found. */

                if (ovector[0] == ovector[1]) {
                    if (ovector[0] == subject.length()) {
                        break;
                    }
                    options = defaultOptions | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
                }
                start = ovector[1] + subject.length() - subjectLength;
            }
            return subject;
        }

        ~Pcre2Private() {
            pcre2_match_data_free(match_data);
            pcre2_code_free(re);
            pcre2_maketables_free(nullptr, table);
            pcre2_compile_context_free(cc);
        };
    private:
        Pcre2Private(pcre2_compile_context *cc, const uint8_t *table, pcre2_code *re, pcre2_match_data *match_data, uint32_t nameCount, PCRE2_SPTR nameTable, uint32_t nameEntrySize) :
            cc(cc),
            table(table),
            re(re),
            match_data(match_data),
            maxBufferSize(256),
            buffer(std::make_unique<PCRE2_UCHAR[]>(maxBufferSize)),
            nameCount(nameCount),
            nameTable(nameTable),
            nameEntrySize(nameEntrySize),
            ovector(pcre2_get_ovector_pointer(match_data)) {
        };
        // 禁止外部拷贝构造
        Pcre2Private(const Pcre2Private& single) = delete;
        // 禁止外部赋值操作
        const Pcre2Private& operator=(const Pcre2Private& single) = delete;
    private:
        pcre2_compile_context *cc;
        const uint8_t *table;
        pcre2_code *re;
        pcre2_match_data *match_data;
        PCRE2_SIZE maxBufferSize;
        std::unique_ptr<PCRE2_UCHAR[]> buffer;
        uint32_t nameCount;
        PCRE2_SPTR nameTable;
        uint32_t nameEntrySize;
        PCRE2_SIZE *ovector;
    };
} // namespace regex

regex::Pcre2Implementation::Pcre2Implementation(const std::string& pattern) : pattern(pattern), d(nullptr) {
}

regex::Pcre2Implementation::~Pcre2Implementation() {
}

std::expected<bool, std::string> regex::Pcre2Implementation::exist(const std::string& text, size_t start) {
    auto result = init();
    if (result.value_or(false)) {
        return d->exist(text, start);
    }
    return std::unexpected(result.error_or("初始化错误！"));
}

std::expected<std::vector<regex::Pcre2Implementation::FindInfo>, std::string> regex::Pcre2Implementation::find(const std::string& text, int times, size_t start) {
    auto result = init();
    if (result.value_or(false)) {
        return d->find(text, times, start);
    }
    return std::unexpected(result.error_or("初始化错误！"));
}

std::expected<std::string, std::string> regex::Pcre2Implementation::replace(const std::string& originalText, const std::string& replacementText, int times, size_t start) {
    auto result = init();
    if (result.value_or(false)) {
        return d->replace(originalText, replacementText, times, start);
    }
    return std::unexpected(result.error_or("初始化错误！"));
}

std::expected<bool, std::string> regex::Pcre2Implementation::init() {
    if (d == nullptr) {
        auto result = Pcre2Private::generate(pattern);
        d = std::unique_ptr<Pcre2Private>(result.value_or(nullptr));
        if (d == nullptr) {
            return std::unexpected(result.error_or("初始化错误！"));
        }
    }
    return true;
}
