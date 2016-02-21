#include "parser.h"

using namespace std;

int main(int argc, char *argv[]) {
    _finddata_t file, file2;
    long lf, lg;
    if ((lf = _findfirst(argv[2], &file)) != -1l) {
        if (file.attrib == _A_SUBDIR) {  
            string s(argv[2]);
            s += "\\*.txt";

            if ((lg = _findfirst(s.c_str(), &file2)) != -1l) {
                string s1(file2.name);
                size_t pos = s1.find('.');
                s1 = s1.substr(0, pos);
                string s2 = s1;
                s1 = argv[2];
                s1 += "\\";
                s1 += s2;
                s1 += ".output";
                //freopen(s1.c_str(), "w", stdout);

                s2 = argv[2];
                s2 += "\\";
                s2 += file2.name;
                fstream* stream2 = new fstream();
                stream2->open(s2.c_str(), ios::in);

                fstream* stream = new fstream();
                stream->open(argv[1], ios::in);
                Parser parser(stream, stream2);
                cout << "Processing " << s2 << endl;
                parser.process();

                while (_findnext(lg, &file2) == 0) {
                    s1 = file2.name;
                    pos = s1.find('.');
                    s1 = s1.substr(0, pos);
                    s2 = s1;
                    s1 = argv[2];
                    s1 += "\\";
                    s1 += s2;
                    s1 += ".output";
                    //freopen(s1.c_str(), "w", stdout);

                    s2 = argv[2];
                    s2 += "\\";
                    s2 += file2.name;
                    stream2 = new fstream();
                    stream2->open(s2.c_str(), ios::in);

                    stream = new fstream();
                    stream->open(argv[1], ios::in);
                    Parser parser2(stream, stream2);
                    cout << "Processing " << s2 << endl;
                    parser2.process();
                }
            }
        } else {
            string s(argv[2]);
            size_t pos = s.find('.');
            s = s.substr(0, pos);
            s += ".output";
            //freopen(s.c_str(), "w", stdout);

            fstream* stream2 = new fstream();
            stream2->open(argv[2], ios::in);
            fstream* stream = new fstream();
            stream->open(argv[1], ios::in);

            Parser parser(stream, stream2);
            cout << "Processing " << argv[2] << endl;
            parser.process();
        }
        _findclose(lf);
    }

    system("pause");
    return 0;
}