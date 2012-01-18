#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;

template <typename Container, typename Iter>
struct srec_parser: qi::grammar<Iter, Container()> {
	srec_parser(): srec_parser::base_type(start) {
		start = qi::no_skip[lines];
		// _a: address length of this line
		// _b: data length of this line
		// _c: start address of this line
		// _d: data of this line
		// _e: line doesn't contain data
		lines = *('S'
			> qi::digit[
				phx::switch_(qi::_1) [
					phx::case_<'3'>(qi::_a = 4),
					phx::case_<'7'>(qi::_a = 4),
					phx::case_<'2'>(qi::_a = 3),
					phx::case_<'8'>(qi::_a = 3),
					phx::default_(qi::_a = 2)
				],
				phx::switch_(qi::_1) [
					phx::case_<'1'>(qi::_e = false),
					phx::case_<'2'>(qi::_e = false),
					phx::case_<'3'>(qi::_e = false),
					phx::default_(qi::_e = true)
				]
			  ]
			> hex_byte[qi::_b = qi::_1 - qi::_a - 1]
			> qi::eps[qi::_c = 0] > qi::repeat(qi::_a)[hex_byte[qi::_c = (qi::_c << 8) | qi::_1]]
			> qi::eps[phx::clear(qi::_d)] > qi::repeat(qi::_b)[hex_byte[phx::push_back(qi::_d, qi::_1)]]
			> hex_byte[
				qi::_pass = qi::_1 == (0xFF & ~phx::accumulate(qi::_d, (qi::_c>>24) + (qi::_c>>16) + (qi::_c>>8) + qi::_c + qi::_b + qi::_a + 1)),
				if_(!qi::_e) [
					phx::resize(qi::_val, qi::_b+qi::_c),
					phx::for_(qi::_b = 0, qi::_b < phx::size(qi::_d), ++qi::_b) [
						qi::_val[qi::_b+qi::_c] = qi::_d[qi::_b]
					]
				]
			  ]
			> +qi::eol);
		hex_byte = qi::uint_parser<unsigned, 16, 2, 2>();
		qi::on_error<qi::fail>
		(
            lines
          , std::cout
			<< phx::val("Error! Expecting ")
			<< qi::_4                               // what failed?
			<< phx::val(" here: \"")
			<< phx::construct<std::string>(qi::_3, qi::_2)   // iterators to error-pos, end
			<< phx::val("\"")
                << std::endl
        );	}
	qi::rule<Iter, Container()> start;
	qi::rule<Iter, Container(), qi::locals<unsigned, unsigned, std::size_t, std::vector<unsigned char>, bool> > lines;
	qi::rule<Iter, unsigned char()> hex_byte;
};

template <typename Container, typename Iter>
Container parse_srec(Iter &first, Iter last) {
	Container c;
	qi::parse(first, last, srec_parser<Container, Iter>(), c);
	return c;
}

template <typename Container, typename Input>
Container parse_srec(const Input &in) {
	typename Input::const_iterator ite = in.begin();
	typename Input::const_iterator end = in.end();
	return parse_srec<Container>(ite, end);
}

