#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <cstdlib>  // for getenv
#include <fcntl.h> // for open()

using namespace std;



void handleCdCommand(const vector<string>& tokens) {
    if (tokens.size() < 2) {
        cerr << "QUASH: cd: missing argument" << endl;
        return;
    }

    if (chdir(tokens[1].c_str()) != 0) {
        perror("QUASH");
    } else {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            setenv("PWD", cwd, 1);  // Set the PWD environment variable
        } else {
            perror("QUASH: Failed to update PWD");
        }
    }
}





void handleLsCommand(const vector<string>& tokens) {
    pid_t pid = fork();

    if (pid == 0) {
        char* args[tokens.size() + 1];
        for (size_t j = 0; j < tokens.size(); j++) {
            args[j] = const_cast<char*>(tokens[j].c_str());
        }
        args[tokens.size()] = nullptr;

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
}



void handlePwdCommand() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        cout << cwd << endl;
    } else {
        perror("pwd");
    }
}





void handleExitCommand() {
    cout << "Exiting..." << endl;
    exit(0);
}




string resolveEnvVariables(const string& input) {
    string result;
    size_t pos = 0;
    while (pos < input.size()) {
        if (input[pos] == '$') {
            size_t endPos = pos + 1;
            while (endPos < input.size() && (isalnum(input[endPos]) || input[endPos] == '_')) {
                ++endPos;
            }
            string varName = input.substr(pos + 1, endPos - pos - 1);
            const char* varValue = getenv(varName.c_str());
            if (varValue) {
                result += varValue;
            }
            pos = endPos;
        } else {
            result += input[pos];
            ++pos;
        }
    }
    return result;
}








void handleExportCommand(const vector<string>& tokens) {
    if (tokens.size() < 2) {
        cerr << "Usage: export KEY=VALUE" << endl;
        return;
    }

    string key = tokens[1].substr(0, tokens[1].find('='));
    string rawValue = tokens[1].substr(tokens[1].find('=') + 1);

    // Resolve any environment variable references in the raw value
    string resolvedValue = resolveEnvVariables(rawValue);

    setenv(key.c_str(), resolvedValue.c_str(), 1);
}







void handleEchoCommand(const vector<string>& tokens) {
    bool isRedirected = false;
    string outputPath;
    for (const auto& token : tokens) {
        if (token == ">" || token == ">>") {
            isRedirected = true;
            break;
        }
    }

    if (!isRedirected) {
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] != ">" && tokens[i] != ">>") {
                if (i > 1) cout << " ";  // Add space between arguments
                cout << resolveEnvVariables(tokens[i]);
            } else {
                break;  // Stop printing when a redirection token is encountered
            }
        }
        cout << endl;
    } else {
        string output;
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] == ">" || tokens[i] == ">>") {
                outputPath = tokens[i + 1]; // Assuming file name exists after ">"
                break;
            } else {
                if (i > 1) output += " "; // Add space between arguments
                output += resolveEnvVariables(tokens[i]);
            }
        }

        // Handle the actual redirection
        int fd;
        if (tokens[tokens.size() - 2] == ">") {
            fd = open(outputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else {  // ">>"
            fd = open(outputPath.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
        }
        if (fd < 0) {
            perror("open");
            return;
        }
        write(fd, output.c_str(), output.length());
        write(fd, "\n", 1);  // Append newline
        close(fd);
    }
}














vector<string> advancedTokenize(const string& input) {
    vector<string> tokens;
    string currentToken;
    char currentQuoteChar = 0; // holds the current quote character, if inside quotes

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if ((c == '"' || c == '\'') && (currentQuoteChar == 0 || currentQuoteChar == c)) {
            // If we're starting or ending a quoted string
            if (currentQuoteChar == c) {
                currentQuoteChar = 0;
            } else {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
                currentQuoteChar = c;
            }
        } else if (c == ' ' && currentQuoteChar == 0) {
            if (!currentToken.empty()) {
                currentToken = resolveEnvVariables(currentToken);  // Resolve env variables
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        } else {
            currentToken += c;
        }
    }

    if (!currentToken.empty()) {
        currentToken = resolveEnvVariables(currentToken);  // Resolve env variables
        tokens.push_back(currentToken);
    }

    return tokens;
}




vector<string> tokenize(const string& input, const string& delimiter) {
    vector<string> tokens;
    size_t start = 0, end = 0;
    while ((end = input.find(delimiter, start)) != string::npos) {
        if (end != start) {  // Avoid empty tokens
            tokens.push_back(input.substr(start, end - start));
        }
        start = end + delimiter.length();
    }
    if (start < input.size()) {  // Add the last token, if any
        tokens.push_back(input.substr(start));
    }
    return tokens;
}

string trimComment(const string& input) {
    size_t pos = input.find('#');
    if (pos != string::npos) {
        return input.substr(0, pos);
    }
    return input;
}

void handleRedirections(const vector<string>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == ">" && i + 1 < tokens.size()) {
            int fd = open(tokens[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, 1); // redirect stdout
            close(fd);
        } else if (tokens[i] == ">>" && i + 1 < tokens.size()) {
            int fd = open(tokens[i + 1].c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
            dup2(fd, 1); // redirect stdout
            close(fd);
        } else if (tokens[i] == "<" && i + 1 < tokens.size()) {
            int fd = open(tokens[i + 1].c_str(), O_RDONLY);
            dup2(fd, 0); // redirect stdin
            close(fd);
        }
    }
}

vector<string> adjustArguments(const vector<string>& tokens) {
    vector<string> adjustedTokens;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == ">" || tokens[i] == ">>" || tokens[i] == "<") {
            i++; // Skip the file name after the redirection symbol
        } else {
            adjustedTokens.push_back(tokens[i]);
        }
    }
    return adjustedTokens;
}

int executeWithPipes(const vector<string>& commands) {
    size_t num_cmds = commands.size();
    int status;
    int i = 0;
    pid_t pid;
    int in, fd[2];

    in = 0;
    for (i = 0; i < num_cmds - 1; ++i) {
        pipe(fd);
        if ((pid = fork()) == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            if (in != 0) {
                dup2(in, 0);
                close(in);
            }
            dup2(fd[1], 1);
            close(fd[0]);

            vector<string> tokens = tokenize(commands[i], " ");
            handleRedirections(tokens);
            tokens = adjustArguments(tokens);
            char* args[tokens.size() + 1];
            for (size_t j = 0; j < tokens.size(); j++) {
                args[j] = const_cast<char*>(tokens[j].c_str());
            }
            args[tokens.size()] = nullptr;

            execvp(args[0], args);
            perror("execvp");
            exit(1); 
        } else {
            wait(NULL);
            close(fd[1]);
            in = fd[0];
        }
    }
    if ((pid = fork()) == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {  // this is the child process
        if (in != 0) {
            dup2(in, 0);
        }

        vector<string> tokens = tokenize(commands[i], " ");
        handleRedirections(tokens);
        tokens = adjustArguments(tokens);
        char* args[tokens.size() + 1];
        for (size_t j = 0; j < tokens.size(); j++) {
            args[j] = const_cast<char*>(tokens[j].c_str());
        }
        args[tokens.size()] = nullptr;

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    }
    wait(NULL);  // wait for the last command to finish
    return 0;  // don't exit the shell
}

int main() {
    // Welcome message
    cout << "Welcome..." << endl;
    
    string input;

    while (true) {
        cout << "[QUASH]$ ";
        getline(cin, input);

        // Trim comments from input
        input = trimComment(input);

        // Ignore empty commands (can happen after trimming comments)
        if (input.empty()) {
            continue;
        }

        vector<string> tokens = advancedTokenize(input);
        
        if (tokens.empty()) {
            continue;
        }
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            tokens[i] = resolveEnvVariables(tokens[i]);
        }

        // Check for built-in commands first
        if (tokens[0] == "export") {
            handleExportCommand(tokens);
            continue;
        }
        
        if (tokens[0] == "echo") {
            handleEchoCommand(tokens);
            continue;
        }

        if (tokens[0] == "ls") {
            handleLsCommand(tokens);
            continue;
        }

        if (tokens[0] == "pwd") {
            handlePwdCommand();
            continue;
        }
        
        if (tokens[0] == "cd") {
            handleCdCommand(tokens);
            continue;
        }
        
        if (tokens[0] == "exit" || tokens[0] == "quit") {
            handleExitCommand();
        }

        // Handle pipes and generic commands only if the input isn't a built-in command
        vector<string> commands = tokenize(input, "|");

        if (commands.size() > 1) {
            executeWithPipes(commands);
        } else {
            pid_t pid = fork();

            if (pid == 0) { // Child process
                handleRedirections(tokens);
                tokens = adjustArguments(tokens);
                char* args[tokens.size() + 1];
                for (size_t j = 0; j < tokens.size(); j++) {
                    args[j] = const_cast<char*>(tokens[j].c_str());
                }
                args[tokens.size()] = nullptr;

                execvp(args[0], args);
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                perror("fork");
                return 1;
            }
        }
        
    }

    return 0;
}

