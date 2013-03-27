#ifndef FILTER_FUZZY_H
#define FILTER_FUZZY_H

NAMESPACE_START

#include "filter.h"
#include <algorithm> // all_of
#include <limits> // numeric limits
#include <vector>

typedef unsigned int lvh_distance;

// taken from: http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
template<class T> static
lvh_distance levenshtein_distance( const T & s1, const T & s2 )
{
    const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<unsigned int> col( len2+1 ), prevCol( len2+1 );

    for( unsigned int i = 0; i < prevCol.size(); i++ )
        prevCol[i] = i;

    for( unsigned int i = 0; i < len1; i++ )
    {
        col[0] = i+1;
        for( unsigned int j = 0; j < len2; j++ )
            col[j+1] = std::min( std::min( 1 + col[j], 1 + prevCol[1 + j] ),
                            prevCol[j] + ( s1[i]==s2[j] ? 0 : 1 ) );
        col.swap( prevCol );
    }
    return prevCol[len2];
}

// -------------------------------------------------------------------------- //

struct fuzzyFilterOption
{
    std::string expression;
    unsigned int numberOfMatch;
};

// -------------------------------------------------------------------------- //

struct filterFuzzy : public filter
{
    filterFuzzy( const std::string & _expression, unsigned int _nbSentencesToKeep )
        :m_expression( _expression )
        ,m_keptSentences()
        ,m_levenshteinValues()
    {
        m_keptSentences.reserve( _nbSentencesToKeep );
        m_levenshteinValues.reserve( _nbSentencesToKeep );
    }

    filterFuzzy( const fuzzyFilterOption & _options )
        :m_expression( _options.expression )
        ,m_keptSentences()
        ,m_levenshteinValues()
    {
        m_keptSentences.reserve( _options.numberOfMatch );
        m_levenshteinValues.reserve( _options.numberOfMatch );
    }

    /**@brief Checks that a sentence is close enough
    * @param[in] _sentence The second sentence.
    * @return True if the passed sentence is linked to the sentence passed in
    * the filterLink constructor */
    bool parse( const sentence & _sentence ) throw() TATO_OVERRIDE
    {
        using namespace qlog;

        assert( m_levenshteinValues.size() == m_keptSentences.size() );
        assert( m_levenshteinValues.capacity() == m_keptSentences.capacity() );

        const lvh_distance distance = levenshtein_distance( m_expression, std::string( _sentence.str() ) );

        // qlog::info << "\tdistance of " << color(green) << _sentence.str() << color() << " and " << color(green) << m_expression << color() << ": " << distance << '\n';
        unsigned int index = 0, maxIndex = 0;
        lvh_distance maxDistance = 0, minDistance = std::numeric_limits<lvh_distance>::max();

        // if the top N values have not been computed then we add one more
        if( m_levenshteinValues.size() < m_levenshteinValues.capacity() )
        {
            // qlog::info << "\tadding distance: " << distance << '\n';
            m_levenshteinValues.push_back( distance );
            m_keptSentences.push_back( _sentence.getId() );
        }
        else
        {
            for( ; index <  m_levenshteinValues.size() ; ++index )
            {
                if( m_levenshteinValues[index] > maxDistance )
                {
                    maxDistance = m_levenshteinValues[index];
                    maxIndex = index;
                }

                if( m_levenshteinValues[index] < minDistance )
                {
                    minDistance = m_levenshteinValues[index];
                }
            }

            if( distance < minDistance )
            {
                // qlog::info << "\treplacing distance: " << m_levenshteinValues[maxIndex]  << " with " << distance << '\n';
                m_levenshteinValues[maxIndex] = distance;
                m_keptSentences[maxIndex] = _sentence.getId();
            }
        }

        return true;
    }

    bool postProcess( const sentence & _sentence ) TATO_OVERRIDE
    {
        return std::find( m_keptSentences.begin(), m_keptSentences.end(), _sentence.getId() ) != m_keptSentences.end();
    }

private:
    const std::string & m_expression;
    std::vector< sentence::id > m_keptSentences;
    std::vector< lvh_distance > m_levenshteinValues;
};

NAMESPACE_END

#endif // FILTER_FUZZY_H