// devtool.cpp — C++17 CLI helpers for web dev
// Build (CMake): cmake -S . -B build && cmake --build build -j
// Build (one-liner): clang++ -std=gnu++17 -O2 -Wall -Wextra devtool.cpp -o devtool

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// --------- utils ----------
static inline string trim(const string& s){
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}
static inline bool starts_with(const string& s, const string& p){ return s.rfind(p,0)==0; }

// --------- UUID v4 ----------
string uuid_v4(){
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dist;
    uint64_t a = dist(gen), b = dist(gen);
    // set version (4) and variant (10)
    a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    stringstream ss; ss<<hex<<nouppercase<<setfill('0');
    ss<<setw(8)<<(uint32_t)(a>>32)<<"-"<<setw(4)<<((a>>16)&0xFFFF)<<"-"<<setw(4)<<(a&0xFFFF)
      <<"-"<<setw(4)<<(uint16_t)(b>>48)<<"-"<<setw(12)<<(b & 0x0000FFFFFFFFFFFFULL);
    return ss.str();
}

// --------- slugify ----------
string slugify(string s){
    string out; out.reserve(s.size());
    for(unsigned char c : s){
        if(isalnum(c)) out.push_back(char(::tolower(c)));
        else out.push_back('-');
    }
    // collapse
    string tmp; bool dash=false;
    for(char c: out){
        if(c=='-'){ if(!dash){ tmp.push_back('-'); dash=true; } }
        else { tmp.push_back(c); dash=false; }
    }
    // trim
    if(!tmp.empty() && tmp.front()=='-') tmp.erase(tmp.begin());
    if(!tmp.empty() && tmp.back ()=='-') tmp.pop_back();
    return tmp;
}

// --------- case converters ----------
string to_kebab(string s){
    string out;
    for(size_t i=0;i<s.size();++i){
        unsigned char c=s[i];
        if(i && isupper(c) && (islower(static_cast<unsigned char>(s[i-1])) || (i+1<s.size() && islower(static_cast<unsigned char>(s[i+1])))))
            out.push_back('-');
        if(c=='_'||c==' ') out.push_back('-');
        else out.push_back(char(::tolower(c)));
    }
    return slugify(out);
}
string to_snake(string s){
    string out;
    for(size_t i=0;i<s.size();++i){
        unsigned char c=s[i];
        if(i && isupper(c) && (islower(static_cast<unsigned char>(s[i-1])) || (i+1<s.size() && islower(static_cast<unsigned char>(s[i+1])))))
            out.push_back('_');
        if(c=='-'||c==' ') out.push_back('_');
        else out.push_back(char(::tolower(c)));
    }
    // collapse underscores
    string tmp; bool u=false;
    for(char c: out){ if(c=='_'){ if(!u){ tmp.push_back('_'); u=true; } } else { tmp.push_back(c); u=false; } }
    if(!tmp.empty()&&tmp.front()=='_') tmp.erase(tmp.begin());
    if(!tmp.empty()&&tmp.back ()=='_') tmp.pop_back();
    return tmp;
}
string to_camel(string s){
    string k = to_kebab(s);
    string out; bool up=false;
    for(char c: k){
        if(c=='-'){ up=true; continue; }
        if(up){ out.push_back(char(::toupper(static_cast<unsigned char>(c)))); up=false; }
        else out.push_back(c);
    }
    return out;
}
string to_pascal(string s){
    string c = to_camel(s);
    if(!c.empty()) c[0] = char(::toupper(static_cast<unsigned char>(c[0])));
    return c;
}

// --------- URL encode/decode ----------
string url_encode(const string& s){
    static const char* hex="0123456789ABCDEF";
    string out; out.reserve(s.size()*3);
    for(unsigned char c: s){
        bool safe = (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~');
        if(safe) out.push_back(char(c));
        else if(c==' ') out.push_back('+');
        else { out.push_back('%'); out.push_back(hex[(c>>4)&0xF]); out.push_back(hex[c&0xF]); }
    }
    return out;
}
int from_hex(char c){
    if(c>='0'&&c<='9') return c-'0';
    if(c>='A'&&c<='F') return c-'A'+10;
    if(c>='a'&&c<='f') return c-'a'+10;
    return -1;
}
string url_decode(const string& s){
    string out; out.reserve(s.size());
    for(size_t i=0;i<s.size();++i){
        char c=s[i];
        if(c=='+') out.push_back(' ');
        else if(c=='%' && i+2<s.size()){
            int h1=from_hex(s[i+1]), h2=from_hex(s[i+2]);
            if(h1>=0 && h2>=0){ out.push_back(char((h1<<4)|h2)); i+=2; }
            else out.push_back(c);
        } else out.push_back(c);
    }
    return out;
}

// --------- Base64 encode/decode ----------
static const string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
string b64_encode(const vector<uint8_t>& bytes){
    string out; out.reserve((bytes.size()+2)/3*4);
    int val=0, valb=-6;
    for(uint8_t c: bytes){
        val=(val<<8)+c; valb+=8;
        while(valb>=0){ out.push_back(B64_CHARS[(val>>valb)&0x3F]); valb-=6; }
    }
    if(valb>-6) out.push_back(B64_CHARS[((val<<8)>>(valb+8))&0x3F]);
    while(out.size()%4) out.push_back('=');
    return out;
}
vector<uint8_t> b64_decode(const string& s){
    vector<int> T(256,-1);
    for(int i=0;i<64;i++) T[(int)B64_CHARS[i]]=i;
    vector<uint8_t> out; out.reserve(s.size()*3/4);
    int val=0, valb=-8;
    for(unsigned char c: s){
        if(T[c]==-1){ if(c=='=' ) break; else continue; }
        val=(val<<6)+T[c]; valb+=6;
        if(valb>=0){ out.push_back(uint8_t((val>>valb)&0xFF)); valb-=8; }
    }
    return out;
}

// --------- .env compare ----------
set<string> parse_env_keys(const string& path){
    ifstream f(path);
    set<string> keys;
    if(!f) return keys;
    string line;
    while(getline(f,line)){
        line = trim(line);
        if(line.empty() || starts_with(line, "#")) continue;
        if(starts_with(line,"export ")) line = line.substr(7);
        auto eq = line.find('=');
        if(eq==string::npos) continue;
        string k = trim(line.substr(0,eq));
        if(!k.empty()) keys.insert(k);
    }
    return keys;
}

// --------- JSON helpers (minify/pretty) ----------
string read_all_stdin() {
    ostringstream ss;
    ss << cin.rdbuf();
    return ss.str();
}
string read_file_or_stdin(const string& path) {
    if (path == "-" || path.empty()) return read_all_stdin();
    ifstream in(path);
    if(!in) return {};
    return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

// Remove whitespace outside strings + // and /* */ comments.
string json_minify_str(const string& s){
    string out; out.reserve(s.size());
    bool in_str=false, esc=false;
    for(size_t i=0;i<s.size();++i){
        char c=s[i];
        if(!in_str){
            // line comment //
            if(c=='/' && i+1<s.size() && s[i+1]=='/'){
                i+=2;
                while(i<s.size() && s[i]!='\n') ++i;
                if(i<s.size()) out.push_back('\n');
                continue;
            }
            // block comment /* ... */
            if(c=='/' && i+1<s.size() && s[i+1]=='*'){
                i+=2;
                while(i+1<s.size() && !(s[i]=='*' && s[i+1]=='/')) ++i;
                ++i; // skip '/'
                continue;
            }
            if(isspace(static_cast<unsigned char>(c))) continue;
            if(c=='"') { in_str=true; esc=false; out.push_back(c); continue; }
            out.push_back(c);
        }else{
            out.push_back(c);
            if(esc) esc=false;
            else if(c=='\\') esc=true;
            else if(c=='"') in_str=false;
        }
    }
    return out;
}

string json_pretty_str(const string& s){
    string m = json_minify_str(s);
    string out; out.reserve(m.size()*2);
    int indent=0;
    bool in_str=false, esc=false;
    auto newline_indent=[&](){
        out.push_back('\n');
        for(int i=0;i<indent;i++) out.append("  "); // 2 spaces
    };
    for(size_t i=0;i<m.size();++i){
        char c=m[i];
        if(in_str){
            out.push_back(c);
            if(esc) esc=false;
            else if(c=='\\') esc=true;
            else if(c=='"') in_str=false;
            continue;
        }
        switch(c){
            case '"': in_str=true; out.push_back(c); break;
            case '{': case '[':
                out.push_back(c); indent++; newline_indent(); break;
            case '}': case ']':
                indent--; newline_indent(); out.push_back(c); break;
            case ',':
                out.push_back(c); newline_indent(); break;
            case ':':
                out.push_back(':'); out.push_back(' '); break;
            default:
                out.push_back(c);
        }
    }
    return out;
}

// --------- bump package.json version (regex-based) ----------
bool bump_version_in_file(const string& path, const string& part){
    ifstream in(path);
    if(!in) return false;
    string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

    // custom-delimited raw string to avoid early R"( ... )" termination
    regex r(R"_RE_("version"\s*:\s*"(\d+)\.(\d+)\.(\d+)")_RE_");
    smatch m;
    if(!regex_search(content, m, r)) return false;

    int major=stoi(m[1].str()), minor=stoi(m[2].str()), patch=stoi(m[3].str());
    if(part=="major"){ major++; minor=0; patch=0; }
    else if(part=="minor"){ minor++; patch=0; }
    else { patch++; }

    string replacement = "\"version\": \"" + to_string(major)+"."+to_string(minor)+"."+to_string(patch) + "\"";
    content = regex_replace(content, r, replacement, regex_constants::format_first_only);

    ofstream out(path, ios::trunc);
    out<<content;
    return true;
}

// --------- CLI help ----------
void print_help(){
    cout <<
R"(devtool — handy CLI helpers for web dev

Usage:
  devtool uuid
  devtool slugify "Some Title…"
  devtool case kebab|snake|camel|pascal "Input Text"
  devtool url encode "A&B czech šílené" | devtool url decode "%C5%A1"
  devtool b64 encode "text" | devtool b64 decode "dGV4dA=="
  devtool env check .env.example .env
  devtool version bump [major|minor|patch] path/to/package.json
  devtool json pretty [file|-]
  devtool json minify [file|-]
)";
}

// --------- main ----------
int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc<2){ print_help(); return 0; }
    string cmd = argv[1];

    if(cmd=="uuid"){
        cout << uuid_v4() << "\n";
        return 0;
    }
    if(cmd=="slugify"){
        if(argc<3){ cerr<<"need text\n"; return 1; }
        cout << slugify(argv[2]) << "\n";
        return 0;
    }
    if(cmd=="case"){
        if(argc<4){ cerr<<"usage: devtool case [kebab|snake|camel|pascal] \"text\"\n"; return 1; }
        string mode=argv[2], text=argv[3];
        if(mode=="kebab")  cout<<to_kebab(text)<<"\n";
        else if(mode=="snake") cout<<to_snake(text)<<"\n";
        else if(mode=="camel") cout<<to_camel(text)<<"\n";
        else if(mode=="pascal") cout<<to_pascal(text)<<"\n";
        else { cerr<<"unknown case\n"; return 1; }
        return 0;
    }
    if(cmd=="url"){
        if(argc<4){ cerr<<"usage: devtool url [encode|decode] text\n"; return 1; }
        string mode=argv[2], text=argv[3];
        if(mode=="encode") cout<<url_encode(text)<<"\n";
        else if(mode=="decode") cout<<url_decode(text)<<"\n";
        else { cerr<<"unknown url mode\n"; return 1; }
        return 0;
    }
    if(cmd=="b64"){
        if(argc<4){ cerr<<"usage: devtool b64 [encode|decode] input\n"; return 1; }
        string mode=argv[2], text=argv[3];
        if(mode=="encode"){
            vector<uint8_t> bytes(text.begin(), text.end());
            cout<<b64_encode(bytes)<<"\n";
        } else if(mode=="decode"){
            auto bytes=b64_decode(text);
            cout<<string(bytes.begin(), bytes.end())<<"\n";
        } else { cerr<<"unknown b64 mode\n"; return 1; }
        return 0;
    }
    if(cmd=="env"){
        if(argc<3){ cerr<<"usage: devtool env check .env.example .env\n"; return 1; }
        string sub=argv[2];
        if(sub=="check"){
            if(argc<5){ cerr<<"usage: devtool env check .env.example .env\n"; return 1; }
            auto ex = parse_env_keys(argv[3]);
            auto real = parse_env_keys(argv[4]);
            vector<string> missing;
            for(const auto& k: ex) if(!real.count(k)) missing.push_back(k);
            if(missing.empty()){ cout<<"All keys present ✅\n"; }
            else{
                cout<<"Missing keys ("<<missing.size()<<"):\n";
                for(auto& k: missing) cout<<"- "<<k<<"\n";
            }
            return 0;
        }
        cerr<<"unknown env subcommand\n"; return 1;
    }
    if(cmd=="version"){
        if(argc<5){ cerr<<"usage: devtool version bump [major|minor|patch] package.json\n"; return 1; }
        string sub=argv[2];
        if(sub!="bump"){ cerr<<"only 'bump' supported\n"; return 1; }
        string part=argv[3]; if(part!="major"&&part!="minor"&&part!="patch") part="patch";
        string path=argv[4];
        if(!bump_version_in_file(path, part)){ cerr<<"failed to bump (couldn't find version)\n"; return 2; }
        cout<<"bumped "<<part<<" in "<<path<<"\n";
        return 0;
    }
    if(cmd=="json"){
        if(argc<3){ cerr<<"usage: devtool json [pretty|minify] [file|-]\n"; return 1; }
        string mode=argv[2];
        string path = (argc>=4? argv[3] : "-");
        string src = read_file_or_stdin(path);
        if(src.empty() && !cin.good()){
            cerr<<"no input\n"; return 1;
        }
        if(mode=="pretty")  cout << json_pretty_str(src) << "\n";
        else if(mode=="minify") cout << json_minify_str(src) << "\n";
        else { cerr<<"unknown json mode\n"; return 1; }
        return 0;
    }

    print_help();
    return 0;
}
