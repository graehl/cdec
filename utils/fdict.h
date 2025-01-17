#ifndef _FDICT_H_
#define _FDICT_H_

#include "config.h"

#include <iostream>
#include <string>
#include <vector>
#include "dict.h"

#ifdef HAVE_CMPH
#include "perfect_hash.h"
#include "string_to.h"
#endif

struct FD {
  // once the FD is frozen, new features not already in the
  // dictionary will return 0
  static void Freeze() {
    frozen_ = true;
  }
  static bool UsingPerfectHashFunction() {
#ifdef HAVE_CMPH
    return hash_;
#else
    return false;
#endif
  }
  static void EnableHash(const std::string& cmph_file) {
#ifdef HAVE_CMPH
    assert(dict_.max() == 0);  // dictionary must not have
                               // been added to
    hash_ = new PerfectHashFunction(cmph_file);
#endif
  }
  static inline int NumFeats() {
#ifdef HAVE_CMPH
    if (hash_) return hash_->number_of_keys();
#endif
    return dict_.max() + 1;
  }
  static inline WordID Convert(const std::string& s) {
#ifdef HAVE_CMPH
    if (hash_) return (*hash_)(s);
#endif
    return dict_.Convert(s, frozen_);
  }
  static inline const std::string& Convert(const WordID& w) {
#ifdef HAVE_CMPH
    if (hash_) {
      static std::string tls;
      tls = to_string(w);
      return tls;
    }
#endif
    return dict_.Convert(w);
  }
  static std::string Convert(WordID const *i,WordID const* e);
  static std::string Convert(std::vector<WordID> const& v);

  // Escape any string to a form that can be used as the name
  // of a weight in a weights file
  static std::string Escape(const std::string& s);
  static Dict dict_;
 private:
  static bool frozen_;
#ifdef HAVE_CMPH
  static PerfectHashFunction* hash_;
#endif
};

#endif
