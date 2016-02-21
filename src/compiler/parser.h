#include "lexer.h"

class Column {
    public:

    vector<Word*> tokens;
    Column() {
    }
    Column(vector<Word*> t) {
        tokens = t;
    }
    void addToken(Word* t) {
        tokens.push_back(t);
    }
    void setTokens(vector<Word*> v) {
        tokens = v;
    }
};

class View {
    public:

    map<string, Column*> columns;
    map<string, int> str_length;
    View() {
    }
    Column* getColumn(string n) {
        return columns[n];
    }
    int getStrLength(string n) {
        return str_length[n];
    }
    void addColumn(string n, Column* r) {
        columns[n] = r;
    }
    void addStrLength(string n, int i) {
        str_length[n] = i;
    }
};

class Parser {
    public:

    vector<vector<Token*>> stmts;
    map<string, View*> views;
    map<string, string> alias_names;
    Lexer lex;
    Tokenizer tokenizer;
    Token* look;
    string view_name;
    View* current_view;
    int stmt_index;
    int extract_mode;
    string total_expression;
    string current_expression;
    vector<Column*> columns_to_add;
    vector<int> strLength_to_add;

    Parser(fstream* stream, fstream* stream2) {
        move(stream);
        preprocess(stream);
        stream->close();
        tokenizer.readDocument(stream2);
    }
    Parser(Lexer l, fstream* stream, fstream* stream2) {
        lex = l;
        move(stream);
        preprocess(stream);
        stream->close();
        tokenizer.readDocument(stream2);
    }
    void move(fstream* stream) {
        look = lex.scan(stream);
    }
    void preprocess(fstream* stream) {
        while (!(*stream).eof()) {
            vector<Token*> stmt;
            while ((*look).tag != ';') {
                stmt.push_back(look);
                move(stream);
            }
            stmts.push_back(stmt);
            move(stream);
        }
    }

    void select_item(int begin, int end) {
        string v_name = alias_names[stmts[stmt_index][begin]->toString()];
        string r_name = stmts[stmt_index][begin + 2]->toString();
        if (stmts[stmt_index][begin + 3]->tag == AS) {
            string nr_name = stmts[stmt_index][begin + 4]->toString();
            current_view->addColumn(nr_name, views[v_name]->getColumn(r_name));
            current_view->addStrLength(nr_name, views[v_name]->getStrLength(r_name));

        } else {
            current_view->addColumn(r_name, views[v_name]->getColumn(r_name));
            current_view->addStrLength(r_name, views[v_name]->getStrLength(r_name));
        }
    }

    void select_list(int begin, int end) {
        int begin2 = begin, end2;
        for (end2 = begin + 1; end2 < end; end2++) {
            if (stmts[stmt_index][end2]->tag == ',') {
                select_item(begin2, end2);
                begin2 = end2 + 1;
            }
        }
        select_item(begin2, end2);
    }

    void from_item(int begin, int end) {
        string original = (*stmts[stmt_index][begin]).toString();
        string alias = (*stmts[stmt_index][begin + 1]).toString();
        alias_names[alias] = original;
    }

    void from_list(int begin, int end) {
        int begin2 = begin, end2;
        for (end2 = begin + 1; end2 < end; end2++) {
            if (stmts[stmt_index][end2]->tag == ',') {
                from_item(begin2, end2);
                begin2 = end2 + 1;
            }
        }
        from_item(begin2, end2);
    }

    void single_group(int begin, int end) {
        int group = stmts[stmt_index][begin + 1]->tag;
        if (columns_to_add[group] != NULL) {
            current_view->addColumn(stmts[stmt_index][end - 1]->toString(), columns_to_add[group]);
            current_view->addStrLength(stmts[stmt_index][end - 1]->toString(), strLength_to_add[group]);
        }
    }

    void group_spec(int begin, int end) {
        int begin2 = begin, end2;
        for (end2 = begin + 1; end2 < end; end2++) {
            if (stmts[stmt_index][end2]->tag == AND) {
                single_group(begin2, end2);
                begin2 = end2 + 1;
            }
        }
        single_group(begin2, end2);
    }

    void name_spec(int begin, int end) {
        if (stmts[stmt_index][begin]->tag == AS) {
            current_view->addColumn(stmts[stmt_index][begin + 1]->toString(), columns_to_add[0]);
            current_view->addStrLength(stmts[stmt_index][begin + 1]->toString(), strLength_to_add[0]);
        } else if (stmts[stmt_index][begin]->tag == RETURN) {
            group_spec(begin + 1, end);
        }
    }

    vector<Word*> atom(int begin, int end) {
        vector<Word*> tokens;
        if (end > begin) {
            if (stmts[stmt_index][begin]->tag == '<' && stmts[stmt_index][end - 1]->tag == '>') {
                if (stmts[stmt_index][begin + 1]->tag == TOKEN) {
                    tokens = tokenizer.tokens;
                    
                    current_expression = "[0-9a-zA-Z]+|[!-/:-@[-`{-~]";
                } else {
                    string alias = stmts[stmt_index][begin + 1]->toString();
                    string v_name = alias_names[alias];
                    string r_name = stmts[stmt_index][begin + 3]->toString();
                    Column* c = views[v_name]->getColumn(r_name);
                    tokens = c->tokens;
                }
            } else if (stmts[stmt_index][begin]->tag == EXPRESSION) {
                vector<vector<int>> result;
                
                current_expression = stmts[stmt_index][begin]->toString();
                current_expression = current_expression.substr(1, current_expression.size() - 2);
                
                string content = tokenizer.getString();
                result = findall(current_expression.c_str(), content.c_str());
                for (int j = 0; j < result.size(); j++) {
                    string s;
                    for (int k = result[j][0]; k < result[j][1]; k++) {
                        s += content[k];
                    }
                    Word* word = new Word(s, TEXT, 0, 0, result[j][0], result[j][1]);

                    int index, i;
                    for (index = 0; index < tokenizer.tokens.size(); index++) {
                        if (word->begin >= tokenizer.tokens[index]->begin && word->begin < tokenizer.tokens[index]->end) {
                            word->index1 = index;

                            for (i = index; i < tokenizer.tokens.size(); i++) {
                                if (word->end > tokenizer.tokens[i]->begin && word->end <= tokenizer.tokens[i]->end) {
                                    word->index2 = i + 1;
                                    break;
                                }
                            }

                            break;
                        }
                    }
                    tokens.push_back(word);
                }
            }
        }
        return tokens;
    }

    vector<Word*> connect(vector<Word*> tokens1, vector<Word*> tokens2) {
        vector<Word*> tokens;

        for (int i = 0; i < tokens1.size(); i++) {
            int j, k, l;
            for (j = 0; j < tokenizer.tokens.size(); j++) {
                if (tokenizer.tokens[j]->end == tokens1[i]->end) {
                    break;
                }
            }
            j++;
            if (j < tokenizer.tokens.size()) {
                for (k = 0; k < tokens2.size(); k++) {
                    if (tokens2[k]->begin == tokenizer.tokens[j]->begin) {
                        string s;
                        for (l = tokens1[i]->begin; l < tokens2[k]->end; l++) s += tokenizer.s[l];
                        Word* word = new Word(s, TEXT, 0, 0, tokens1[i]->begin, tokens2[k]->end);
                        word->index1 = tokens1[i]->index1;
                        word->index2 = tokens2[k]->index2;
                        tokens.push_back(word);
                    }
                }
            }
        }
        return tokens;
    }

    vector<Word*> extent(vector<Word*> tokens1, int min, int max) {
        vector<Word*> tokens;
        for (int i = 0; i < tokens1.size(); i++) {
            int j, k, l;
            int b, e;
            bool flag = true;
            for (k = 0; k < tokenizer.tokens.size(); k++) {
                if (tokenizer.tokens[k]->begin == tokens1[i]->begin) {
                    break;
                }
            }
            if (i + min >= tokens1.size())
                continue;
            for (j = i; j < i + min; j++) {
                if (k < tokenizer.tokens.size()) {
                    if (tokenizer.tokens[k]->begin != tokens1[j]->begin) {
                        flag = false;
                        break;
                    } else {
                        for (l = k; l < tokenizer.tokens.size(); l++) {
                            if (tokenizer.tokens[l]->end == tokens1[j]->end) {
                                break;
                            }
                        }
                    }
                } else {
                    flag = false;
                    break;
                }
                k = l + 1;
            }
            if (flag) {
                string s;
                for (b = tokens1[i]->begin; b < tokenizer.tokens[k - 1]->end; b++) s += tokenizer.s[b];
                Word* word = new Word(s, TEXT, 0, 0, tokens1[i]->begin, tokenizer.tokens[k - 1]->end);
                tokens.push_back(word);

                for (j = i + min; j < i + max && j < tokens1.size(); j++) {
                    if (k < tokenizer.tokens.size()) {
                        if (tokenizer.tokens[k]->begin != tokens1[j]->begin) {
                            break;
                        } else {
                            for (l = k; l < tokenizer.tokens.size(); l++) {
                                if (tokenizer.tokens[l]->end == tokens1[j]->end) {
                                    s = "";
                                    for (b = tokens1[i]->begin; b < tokenizer.tokens[l]->end; b++) s += tokenizer.s[b];
                                    Word* word = new Word(s, TEXT, 0, 0, tokens1[i]->begin, tokenizer.tokens[l]->end);
                                    tokens.push_back(word);
                                    break;
                                }
                            }
                        }
                    } else {
                        break;
                    }
                    k = l + 1;
                }
            }
        }
        return tokens;
    }

    vector<Word*> backward_connect(vector<Word*> tokens1, int min, int max) {
        stack<Word*> st;
        vector<Word*> tokens;
        for (int i = 0; i < tokens1.size(); i++) {
            if (min <= tokens1[i]->index1) {
                int index = tokens1[i]->index1 - min;
                string s;
                for (int j = tokenizer.tokens[index]->begin; j < tokens1[i]->end; j++) s += tokenizer.s[j];
                Word* word = new Word(s, TEXT, 0, 0, tokenizer.tokens[index]->begin, tokens1[i]->end);
                st.push(word);

                if (tokens1[i]->index1 <= max) {
                    for (int j = min + 1; j <= tokens1[i]->index1; j++) {
                        index = tokens1[i]->index1 - j;
                        s = "";
                        for (int k = tokenizer.tokens[index]->begin; k < tokens1[i]->end; k++) s += tokenizer.s[k];
                        word = new Word(s, TEXT, 0, 0, tokenizer.tokens[index]->begin, tokens1[i]->end);
                        st.push(word);
                    }
                } else if (max < tokens1[i]->index1) {
                    for (int j = min + 1; j <= max; j++) {
                        index = tokens1[i]->index1 - j;
                        s = "";
                        for (int k = tokenizer.tokens[index]->begin; k < tokens1[i]->end; k++) s += tokenizer.s[k];
                        word = new Word(s, TEXT, 0, 0, tokenizer.tokens[index]->begin, tokens1[i]->end);
                        st.push(word);
                    }
                }
            }
        }

        while (!st.empty()) {
            tokens.push_back(st.top());
            st.pop();
        }
        return tokens;
    }

    vector<Word*> pattern_expr(int begin, int end) {
        if (stmts[stmt_index][begin]->tag == '(') {
            int index = begin + 1;
            while (index < end) {
                if ((*stmts[stmt_index][index]).tag == ')') break;
                index += 1;
            }

            if (index < end - 1) {
                vector<Word*> tokens1 = pattern_expr(begin + 1, index);
                Column* c = new Column(tokens1);
                columns_to_add.push_back(c);
                vector<Word*> tokens;
                vector<Word*> tokens2 = pattern_expr(index + 1, end);

                tokens = connect(tokens1, tokens2);
                return tokens;
            } else {
                vector<Word*> tokens1 = pattern_expr(begin + 1, index);
                Column* c = new Column(tokens1);
                columns_to_add.push_back(c);
                return tokens1;
            }
        } else if (stmts[stmt_index][begin]->tag == '<') {
            int index = begin + 1;
            while (index < end) {
                if ((*stmts[stmt_index][index]).tag == '>') break;
                index += 1;
            }

            if (stmts[stmt_index][index + 1]->tag == '{' && stmts[stmt_index][index + 5]->tag == '}') {
                vector<Word*> tokens, tokens3;
                int minTimes = stmts[stmt_index][index + 2]->tag;
                int maxTimes = stmts[stmt_index][index + 4]->tag;

                if (index + 5 < end - 1) {
                    vector<Word*> tokens2 = pattern_expr(index + 6, end);

                    if (stmts[stmt_index][begin + 1]->tag == TOKEN) {
                        tokens = backward_connect(tokens2, minTimes, maxTimes);
                    } else {
                        vector<Word*> tokens1 = atom(begin, index + 1);
                        tokens3 = extent(tokens1, minTimes, maxTimes);
                        tokens = connect(tokens3, tokens2);
                    }
                    
                    return tokens;
                } else {
                    vector<Word*> tokens1 = atom(begin, index + 1);
                    tokens3 = extent(tokens1, minTimes, maxTimes);
                    return tokens3;
                }
            } else {
                vector<Word*> tokens1 = atom(begin, index + 1);
                if (index < end - 1) {
                    vector<Word*> tokens2 = pattern_expr(index + 1, end);
                    vector<Word*> tokens = connect(tokens1, tokens2);
                    return tokens;
                } else {
                    return tokens1;
                }
            }
        } else if (stmts[stmt_index][begin]->tag == EXPRESSION) {
            vector<Word*> tokens1 = atom(begin, begin + 1);

            if (stmts[stmt_index][begin + 1]->tag == '{' && stmts[stmt_index][begin + 5]->tag == '}') {
                vector<Word*> tokens, tokens3;
                int minTimes = stmts[stmt_index][begin + 2]->tag;
                int maxTimes = stmts[stmt_index][begin + 4]->tag;

                tokens3 = extent(tokens1, minTimes, maxTimes);

                if (begin + 5 < end - 1) {
                    vector<Word*> tokens2 = pattern_expr(begin + 6, end);
                    vector<Word*> tokens = connect(tokens3, tokens2);
                    return tokens;
                } else {
                    return tokens3;
                }
            } else {
                if (begin < end - 1) {
                    vector<Word*> tokens2 = pattern_expr(begin + 1, end);
                    vector<Word*> tokens = connect(tokens1, tokens2);
                    return tokens;
                } else {
                    return tokens1;
                }
            }
        }
    }

    void extract_spec(int begin, int end) {
        if (stmts[stmt_index][begin]->tag == REGEX) {
            total_expression = stmts[stmt_index][begin + 1]->toString();
            string alias = stmts[stmt_index][begin + 3]->toString();
            string name = alias_names[alias];

            if (name == "Document" && stmts[stmt_index][begin + 5]->toString() == "text") {
                Column* column = new Column();
                vector<vector<int>> result;
                string content = tokenizer.getString();
                total_expression = total_expression.substr(1, total_expression.size() - 2);

                int index, i;
                result = findall(total_expression.c_str(), content.c_str());
                for (int j = 0; j < result.size(); j++) {
                    string s;
                    for (int k = result[j][0]; k < result[j][1]; k++) {
                        s += content[k];
                    }
                    Word* word = new Word(s, TEXT, 0, 0, result[j][0], result[j][1]);

                    for (index = 0; index < tokenizer.tokens.size(); index++) {
                        if (word->begin >= tokenizer.tokens[index]->begin && word->begin < tokenizer.tokens[index]->end) {
                            word->index1 = index;

                            for (i = index; i < tokenizer.tokens.size(); i++) {
                                if (word->end > tokenizer.tokens[i]->begin && word->end <= tokenizer.tokens[i]->end) {
                                    word->index2 = i + 1;
                                    break;
                                }
                            }

                            break;
                        }
                    }

                    column->addToken(word);
                }
                columns_to_add.push_back(column);
                int maxLength = 0;
                for (int i = 0; i < column->tokens.size(); i++) {
                    if (maxLength < column->tokens[i]->toString().length())
                        maxLength = column->tokens[i]->toString().length();
                }
                strLength_to_add.push_back(maxLength);

                name_spec(begin + 6, end);
            }
        } else if (stmts[stmt_index][begin]->tag == PATTERN) {
            int index = begin + 1;
            while (index < end) {
                if (stmts[stmt_index][index]->tag == AS || stmts[stmt_index][index]->tag == RETURN) break;
                index += 1;
            }
            total_expression = "";
            vector<Word*> total = pattern_expr(begin + 1, index);
            columns_to_add.insert(columns_to_add.begin(), new Column(total));
            int maxLength = 0;
            for (int i = 0; i < total.size(); i++) {
                if (maxLength < total[i]->toString().length())
                    maxLength = total[i]->toString().length();
            }
            strLength_to_add.push_back(maxLength);

            vector<vector<Word*>> v;
            vector<int> indexs;
            for (int j = 0; j < columns_to_add.size(); j++) {
                vector<Word*> result;
                v.push_back(result);
                indexs.push_back(0);
                strLength_to_add.push_back(0);
            }
            for (int i = 0; i < columns_to_add[0]->tokens.size(); i++) {
                int b = columns_to_add[0]->tokens[i]->begin;
                int e = columns_to_add[0]->tokens[i]->end;
                int index2 = b;

                for (int j = 1; j < columns_to_add.size(); j++) {
                    vector<Word*> original = columns_to_add[j]->tokens;

                    for (; indexs[j] < original.size(); indexs[j]++) {
                        if (original[indexs[j]]->begin >= index2 && original[indexs[j]]->end <= e) {
                            v[j].push_back(original[indexs[j]]);
                            index2 = original[indexs[j]]->end;
                            if (strLength_to_add[j] < original[indexs[j]]->toString().length()) {
                                strLength_to_add[j] = original[indexs[j]]->toString().length();
                            }
                            break;
                        }
                    }
                }
            }
            for (int i = 1; i < columns_to_add.size(); i++) {
                columns_to_add[i] = new Column(v[i]);
            }

            name_spec(index, end);
        }
    }

    void view_stmt(int begin, int end) {
        int index = begin + 1;
        while (index < end) {
            if ((*stmts[stmt_index][index]).tag == FROM) break;
            index += 1;
        }
        from_list(index + 1, end);
        columns_to_add.clear();
        strLength_to_add.clear();

        if ((*stmts[stmt_index][begin]).tag == SELECT) {
            if (index < end) {
                select_list(begin + 1, index);
            }
        } else if ((*stmts[stmt_index][begin]).tag == EXTRACT) {
            if (index < end) {
                extract_spec(begin + 1, index);
            }
        }
    }

    void create_stmt(int begin, int end) {
        int index;
        current_view = new View();
        views[view_name] = current_view;

        view_stmt(begin + 4, end);
    }

    void output_stmt(int begin, int end) {
        string original_name = stmts[stmt_index][begin + 2]->toString();
        if (end > 3 && stmts[stmt_index][begin + 3]->tag == AS) {
            string alias = stmts[stmt_index][begin + 4]->toString();
            cout << "View: " << alias << endl;
        } else {
            cout << "View: " << original_name << endl;
        }

        View* v = views[original_name];
        map<string, Column*>::iterator it;
        cout << "+";
        for (it = v->columns.begin(); it != v->columns.end(); it++) {
            int length = v->str_length[it->first];
            for (int j = 0; j < length + 20; j++)  cout << "-";
            cout << "+";
        }
        cout << endl;

        cout << "|";
        for (it = v->columns.begin(); it != v->columns.end(); it++) {
            int length = v->str_length[it->first];
            cout << std::left << setw(length + 20) << it->first;
            cout << "|";
        }
        cout << endl;

        cout << "+";
        for (it = v->columns.begin(); it != v->columns.end(); it++) {
            int length = v->str_length[it->first];
            for (int j = 0; j < length + 20; j++)  cout << "-";
            cout << "+";
        }
        cout << endl;

        it = v->columns.begin();
        int tokens_num = it->second->tokens.size();
        string s, s1;
        stringstream ss;
        for (int i = 0; i < tokens_num; i++) {
            cout << "|";
            for (it = v->columns.begin(); it != v->columns.end(); it++) {
                int length = v->str_length[it->first];
                s = it->second->tokens[i]->toString() + ":(";
                ss << it->second->tokens[i]->begin;
                ss >> s1;
                ss.clear();
                s += s1;
                s += ",";
                ss << it->second->tokens[i]->end;
                ss >> s1;
                ss.clear();
                s += s1;
                s += ")";
                cout << std::left << setw(length + 20) << s;
                cout << "|";
            }
            cout << endl;
        }

        cout << "+";
        for (it = v->columns.begin(); it != v->columns.end(); it++) {
            int length = v->str_length[it->first];
            for (int j = 0; j < length + 20; j++)  cout << "-";
            cout << "+";
        }
        cout << endl;

        if (tokens_num) cout << tokens_num << " rows in set" << endl << endl;
        else cout << "Empty set" << endl << endl;
    }

    void process() {
        for (stmt_index = 0; stmt_index < stmts.size(); stmt_index++) {
            int stmt_length = stmts[stmt_index].size();
            int begin = 0, end = stmt_length, index = 0;
            view_name = (*stmts[stmt_index][2]).toString();

            if ((*stmts[stmt_index][0]).tag == CREATE) {
                create_stmt(0, end);
            } else if ((*stmts[stmt_index][0]).tag == OUTPUT) {
                output_stmt(0, end);
            }
        }
    }
};