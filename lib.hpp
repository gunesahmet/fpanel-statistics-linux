#include <memory>
#include <algorithm>
#include <string>

using I = std::string::iterator;

template <typename T>
bool match(const T& x, I& first, I last) {
	// require x.pat is defined and it's a seq   
	bool is_eq = std::equal(begin(x.pat), end(x.pat), first);
	if (is_eq) {
		std::advance(first, x.pat.size());
	}
	return is_eq;
}

class parser_t {
public:
  template <typename T>
  parser_t(T x) : self_(std::make_shared<model<T>>(std::move(x))) { }

  friend bool match(const parser_t& x, I& first, I last)
  { 
	return x.self_->match_(first, last); 
  }

private:
	struct concept_t {
		virtual ~concept_t() = default;
		virtual bool match_(I& first, I last) const = 0;
	};

	template <typename T>
	struct model : concept_t {
		model(T x) : data_(std::move(x)) { }
		bool match_(I& first, I last) const
		{ return match(data_, first, last); }
		T data_;
	};

	std::shared_ptr<const concept_t> self_;
};