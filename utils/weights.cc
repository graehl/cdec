#include "weights.h"

#include <sstream>

#include "fdict.h"
#include "filelib.h"
#include "verbose.h"

using namespace std;

void Weights::InitFromFile(const string& filename,
                           vector<weight_t>* pweights,
                           vector<string>* feature_list) {
  vector<weight_t>& weights = *pweights;
  if (!SILENT) cerr << "Reading weights from " << filename << endl;
  ReadFile in_file(filename);
  istream& in = *in_file.stream();
  assert(in);
  
  bool read_text = true;
  if (1) {
    ReadFile hdrrf(filename);
    istream& hi = *hdrrf.stream();
    assert(hi);
    char buf[10];
    hi.read(buf, 5);
    assert(hi.good());
    if (strncmp(buf, "_PHWf", 5) == 0) {
      read_text = false;
    }
  }

  if (read_text) {
    int weight_count = 0;
    bool fl = false;
    string buf;
    weight_t val = 0;
    while (in) {
      getline(in, buf);
      if (buf.size() == 0) continue;
      if (buf[0] == '#') continue;
      if (buf[0] == ' ') {
        cerr << "Weights file lines may not start with whitespace.\n" << buf << endl;
        abort();
      }
      for (int i = buf.size() - 1; i > 0; --i)
        if (buf[i] == '=' || buf[i] == '\t') { buf[i] = ' '; break; }
      int start = 0;
      while(start < buf.size() && buf[start] == ' ') ++start;
      int end = 0;
      while(end < buf.size() && buf[end] != ' ') ++end;
      const int fid = FD::Convert(buf.substr(start, end - start));
      if (feature_list) { feature_list->push_back(buf.substr(start, end - start)); }
      while(end < buf.size() && buf[end] == ' ') ++end;
      val = strtod(&buf.c_str()[end], NULL);
      if (isnan(val)) {
        cerr << FD::Convert(fid) << " has weight NaN!\n";
        abort();
      }
      if (weights.size() <= fid)
        weights.resize(fid + 1);
      weights[fid] = val;
      ++weight_count;
      if (!SILENT) {
        if (weight_count %   50000 == 0) { cerr << '.' << flush; fl = true; }
        if (weight_count % 2000000 == 0) { cerr << " [" << weight_count << "]\n"; fl = false; }
      }
    }
    if (!SILENT) {
      if (fl) { cerr << endl; }
      cerr << "Loaded " << weight_count << " feature weights\n";
    }
  } else {   // !read_text
    char buf[6];
    in.read(buf, 5);
    size_t num_keys;
    in.read(reinterpret_cast<char*>(&num_keys), sizeof(size_t));
    if (num_keys != FD::NumFeats()) {
      cerr << "Hash function reports " << FD::NumFeats() << " keys but weights file contains " << num_keys << endl;
      abort();
    }
    weights.resize(num_keys);
    in.read(reinterpret_cast<char*>(&weights.front()), num_keys * sizeof(weight_t));
    if (!in.good()) {
      cerr << "Error loading weights!\n";
      abort();
    } else {
      cerr << "  Successfully loaded " << (num_keys * sizeof(weight_t)) << " bytes\n";
    }
  }
}

void Weights::WriteToFile(const string& fname,
                          const vector<weight_t>& weights,
                          bool hide_zero_value_features,
                          const string* extra) {
  WriteFile out(fname);
  ostream& o = *out.stream();
  assert(o);
  bool write_text = !FD::UsingPerfectHashFunction();

  if (write_text) {
    if (extra) { o << "# " << *extra << endl; }
    o.precision(17);
    const int num_feats = FD::NumFeats();
    for (int i = 1; i < num_feats; ++i) {
      const weight_t val = (i < weights.size() ? weights[i] : 0.0);
      if (hide_zero_value_features && val == 0.0) continue;
      o << FD::Convert(i) << ' ' << val << endl;
    }
  } else {
    o.write("_PHWf", 5);
    const size_t keys = FD::NumFeats();
    assert(keys <= weights.size());
    o.write(reinterpret_cast<const char*>(&keys), sizeof(keys));
    o.write(reinterpret_cast<const char*>(&weights[0]), keys * sizeof(weight_t));
  }
}

void Weights::InitSparseVector(const vector<weight_t>& dv,
                               SparseVector<weight_t>* sv) {
  sv->clear();
  for (unsigned i = 1; i < dv.size(); ++i) {
    if (dv[i]) sv->set_value(i, dv[i]);
  }
}

void Weights::SanityCheck(const vector<weight_t>& w) {
  for (int i = 0; i < w.size(); ++i) {
    assert(!isnan(w[i]));
    assert(!isinf(w[i]));
  }
}

struct FComp {
  const vector<weight_t>& w_;
  FComp(const vector<weight_t>& w) : w_(w) {}
  bool operator()(int a, int b) const {
    return fabs(w_[a]) > fabs(w_[b]);
  }
};

void Weights::ShowLargestFeatures(const vector<weight_t>& w) {
  vector<int> fnums(w.size());
  for (int i = 0; i < w.size(); ++i)
    fnums[i] = i;
  vector<int>::iterator mid = fnums.begin();
  mid += (w.size() > 10 ? 10 : w.size());
  partial_sort(fnums.begin(), mid, fnums.end(), FComp(w));
  cerr << "TOP FEATURES:";
  for (vector<int>::iterator i = fnums.begin(); i != mid; ++i) {
    cerr << ' ' << FD::Convert(*i) << '=' << w[*i];
  }
  cerr << endl;
}


