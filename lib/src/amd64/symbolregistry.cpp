#include <amd64/symbolregistry.hpp>

namespace Amd64 {
SymbolRegistry::SymbolRegistry() {

}

void SymbolRegistry::add(const std::string &name, Symbol symbol) {
  auto r = this->m_symbols.insert({ name, symbol });

  if (!r.second) {
     r.first->second = symbol;
  }
}

void SymbolRegistry::remove(const std::string &name) {
  this->m_symbols.erase(name);
}

Symbol SymbolRegistry::get(const std::string &name) {
  auto it = this->m_symbols.find(name);
  if (it == this->m_symbols.end()) {
    throw std::out_of_range("Missing symbol " + name);
  }

  return it->second;
}

bool SymbolRegistry::has(const std::string &name) {
  return this->m_symbols.find(name) != this->m_symbols.end();
}
}
