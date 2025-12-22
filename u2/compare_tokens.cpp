#include <bits/stdc++.h>
using namespace std;

static void print_context(const vector<string>& v, size_t idx, size_t context = 3) {
    size_t start = (idx < context) ? 0 : idx - context;
    size_t end = min(v.size(), idx + context + 1);
    for (size_t i = start; i < end; ++i) {
        if (i == idx) cout << ">>[" << i+1 << "]\"" << v[i] << "\"<< ";
        else cout << "[" << i+1 << "]\"" << v[i] << "\" ";
    }
    cout << '\n';
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " expected_file actual_file\n";
        return 2;
    }

    ifstream fe(argv[1]), fa(argv[2]);
    if (!fe) {
        cerr << "Error opening expected file: " << argv[1] << '\n';
        return 2;
    }
    if (!fa) {
        cerr << "Error opening actual file: " << argv[2] << '\n';
        return 2;
    }

    vector<string> exp_tokens, act_tokens;
    // Read tokens (whitespace separated)
    copy(istream_iterator<string>(fe), istream_iterator<string>(), back_inserter(exp_tokens));
    copy(istream_iterator<string>(fa), istream_iterator<string>(), back_inserter(act_tokens));

    size_t n_exp = exp_tokens.size();
    size_t n_act = act_tokens.size();
    size_t n_min = min(n_exp, n_act);

    for (size_t i = 0; i < n_min; ++i) {
        if (exp_tokens[i] != act_tokens[i]) {
            cout << "First difference at token " << (i+1) << ":\n";
            cout << "  expected: \"" << exp_tokens[i] << "\"\n";
            cout << "  actual:   \"" << act_tokens[i]  << "\"\n\n";
            cout << "Context in expected file:\n";
            print_context(exp_tokens, i);
            cout << "Context in actual file:\n";
            print_context(act_tokens, i);
            cout << "\nSummary: expected tokens = " << n_exp << ", actual tokens = " << n_act << '\n';
            return 1;
        }
    }

    if (n_exp != n_act) {
        cout << "Files differ in token count.\n";
        cout << "Expected tokens: " << n_exp << "\n";
        cout << "Actual tokens:   " << n_act  << "\n";
        if (n_exp > n_act) {
            cout << "Extra tokens in expected starting at token " << (n_act + 1) << ":\n";
            size_t show = min<size_t>(5, n_exp - n_act);
            for (size_t i = 0; i < show; ++i)
                cout << "  [" << (n_act + 1 + i) << "] \"" << exp_tokens[n_act + i] << "\"\n";
        } else {
            cout << "Extra tokens in actual starting at token " << (n_exp + 1) << ":\n";
            size_t show = min<size_t>(5, n_act - n_exp);
            for (size_t i = 0; i < show; ++i)
                cout << "  [" << (n_exp + 1 + i) << "] \"" << act_tokens[n_exp + i] << "\"\n";
        }
        return 1;
    }

    cout << "Files are identical token-by-token (" << n_exp << " tokens).\n";
    return 0;
}
