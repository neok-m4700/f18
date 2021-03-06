// Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FORTRAN_PARSER_INSTRUMENTED_PARSER_H_
#define FORTRAN_PARSER_INSTRUMENTED_PARSER_H_

#include "message.h"
#include "parse-state.h"
#include "provenance.h"
#include "user-state.h"
#include <cstddef>
#include <map>
#include <ostream>

namespace Fortran::parser {

class ParsingLog {
public:
  ParsingLog() {}

  void clear();

  bool Fails(const char *at, const MessageFixedText &tag, ParseState &);
  void Note(const char *at, const MessageFixedText &tag, bool pass,
      const ParseState &);
  void Dump(std::ostream &, const CookedSource &) const;

private:
  struct LogForPosition {
    struct Entry {
      Entry() {}
      bool pass{true};
      int count{0};
      bool deferred{false};
      Messages messages;
    };
    std::map<MessageFixedText, Entry> perTag;
  };
  std::map<std::size_t, LogForPosition> perPos_;
};

template<typename PA> class InstrumentedParser {
public:
  using resultType = typename PA::resultType;
  constexpr InstrumentedParser(const InstrumentedParser &) = default;
  constexpr InstrumentedParser(const MessageFixedText &tag, const PA &parser)
    : tag_{tag}, parser_{parser} {}
  std::optional<resultType> Parse(ParseState &state) const {
    if (UserState * ustate{state.userState()}) {
      if (ParsingLog * log{ustate->log()}) {
        const char *at{state.GetLocation()};
        if (log->Fails(at, tag_, state)) {
          return {};
        }
        Messages messages{std::move(state.messages())};
        std::optional<resultType> result{parser_.Parse(state)};
        log->Note(at, tag_, result.has_value(), state);
        messages.Annex(state.messages());
        state.messages() = std::move(messages);
        return result;
      }
    }
    return parser_.Parse(state);
  }

private:
  const MessageFixedText tag_;
  const PA parser_;
};

template<typename PA>
inline constexpr auto instrumented(
    const MessageFixedText &tag, const PA &parser) {
  return InstrumentedParser{tag, parser};
}

}  // namespace Fortran::parser
#endif  // FORTRAN_PARSER_INSTRUMENTED_PARSER_H_
