/* Copyright (C) 2013 Webyog Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/

#include "Datatype.h"
#include "wyString.h"
#include "pcre.h"


#include <string>
//
#define PCRE_COMPLIE_OPTIONS            PCRE_UTF8|PCRE_CASELESS|PCRE_NEWLINE_CR


//patterns
#define TABLEPAT_WTBKQUOTES			"^`[^`]+`\\s*\\.\\s*(`[^`]+`|[^\\s|^`|^,]+)|(^`[^`]+`)"
#define TABLEPAT_WTOUTBKQUOTES		"^`[^\\s|^`|^,|^\\.]+\\s*\\.\\s*(`[^`]+`|[^\\s|^`|^,]+)|(^`[^\\s|^`|^,]+)"
#define TABLEPAT_WTOUTBKQUOTES_INS	"^`[^\\s|^`|^,|^\\.]+\\s*\\.\\s*(`[^`]+`|[^\\s|^`|^,|^\\(]+)|(^`[^\\s|^`|^,|^\\(]+)"
#define FUNASCOL_WTOUTBKQUOTES		"^`[^\\(|^`|^\\s|^,|^\"|^\'|^\\.]+(\\s*\\.\\s*(`[^`]+`|[^\\(|^`|^\\s|^,|^\"|^\'|^\\.]+))?\\s*\\("
#define FUNASCOL_WTBKQUOTES			"^`[^`]+`(\\s*\\.\\s*(`[^`]+`|[^\\(|^`|^\\s|^,|^\"|^\'|^\\.]+))?\\s*\\("
#define TABLEALIAS					"^AS\\s*(`[^`]+`|[^,|^\\s]+)|(^`[^`]+`|[^,|^\\s]+)"
#define WHITESPACE					"(\\s,|\\n{1,}\\s*|\\t{1,}\\s*|\\h{1,}\\s+)"
#define JOINCOND_CLAUSE				"^ON[`\\(\\s]|^USING[\\(\\s]"



#define SELECT_LMTCLAUSES       "^(WHERE|GROUP\\s+BY|ORDER\\s+BY|HAVING|LIMIT)[`\\(\\s]"
#define DELETE_LMTCLAUSES		"^(WHERE|ORDER\\s+BY|LIMIT)[`\\(\\s]"
#define GROUPBY_CLAUSE          "(GROUP\\s+BY|HAVING|ORDER\\s+BY)[`\\(\\s]|\\(\\s*SELECT|LIMIT\\s"
#define HAVING_CLAUSE           "(HAVING|ORDER\\s+BY)[`\\(\\s]|\\(\\s*SELECT|LIMIT\\s"
#define ORDERBY_CLAUSE          "ORDER\\s+BY[`\\(\\s]|\\(\\s*SELECT|LIMIT\\s"
#define LIMIT_CLAUSE            "\\(\\s*SELECT|LIMIT\\s"


#define SELECT_OPTS             "(\\s+ALL|\\s+DISTINCT|\\s+DISTINCTROW)?(\\s+HIGH_PRIORITY)?(\\s+STRAIGHT_JOIN)?(\\s+SQL_SMALL_RESULT)?\
(\\s+SQL_BIG_RESULT)?(\\s+SQL_BUFFER_RESULT)?(\\s+SQL_CACHE|\\s+SQL_NO_CACHE)?(\\s+SQL_CALC_FOUND_ROWS)?"



//operators
#define MYSQL_OPERATORS              "<=>|>=|<=|<>|!=|&&|:=|\\|\\||[=><!~%&-]|\\*|\\+|\\^|\\|\\/"



//
#define OVECCOUNT             30                                 /* should be a multiple of 3 */
#define ZERO_LENGTH           0
#define MATCHED               1
#define NOTMATCHED            -1
#define CONTINUE_PARSE        2
#define MAIN_QUERY            0
#define SUB_QUERY             1
#define CONDSUB_QUERY         2
//

//Different patterns which come after table name

#define GROUPBY                 1
#define ORDERBY                 2
#define SUBQUERY                3
#define HAVING                  4
#define LIMIT                   5

#define SPACEINDENTATION       2
#define SELECTKWLENGTH	       6
#define FROMKWLENGTH	       4
#define WHEREKWLENGTH	       5
#define ORDERBYKWLENGTH	       8
#define GROUPBYKWLENGTH	       8
#define EXPLAINKWLENGTH	       7
#define SPACEAFTERKW	       1
#define ONCLAUSELS             2


#define BEFORECOMMA            0
#define AFTERCOMMA             1

//statement type
#define	SELECT_STMT         1
#define INSERT_STMT         2
#define	UPDATE_STMT			3
#define DELETE_STMT			4

//field length will useful when fun or subquery is used as a col 
struct ColumnList
{
	wyChar  *m_col;	
    wyChar  *m_alias;
	wyChar  *m_askw;
	wyInt32 m_collength;
};

struct FormatOpts
{
	wyInt32	m_spaceindentation;
	wyBool  m_stacked;
	wyInt32 m_linebreak; 
	wyBool  m_colalign;

};

//buffer contain comment name , staring positiion and leadspaces. leadspaces
//field indicates how many resultant spaces to be adding before comments.
//if query is trimmed we have to move comment posion to previous to trimmed count
//, if spaces are additionaaly addded we have to move comment pos back
struct Comments
{
	wyInt32  m_startpos; 
	wyChar   *m_comment;	
	wyInt32  m_leadspaces;  
};



class SQLFormatter
{
public:

	/// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	SQLFormatter();
	
	/// Default destructor
    /**
    Free up all the allocated resources
    */
	~SQLFormatter();

    ///matching the query for the table
	/**
	@param query     :OUT formatting query	
	@return void.
	*/
	void     MatchTablePattern(wyString *query);
	///matching the query for the table alias
	/**
	@param query     :OUT formatting query	
	@return void.
	*/
	void     MatchTableAliasPattern(wyString *query);
	///set the pattern for matching select keyword and select options
	/**
	@param query     :OUT formatting query	
	@return void.
	*/
	void     SetSelectPattern(wyString *selpat, wyInt32 querytype);	
	wyBool   DoDMSSqlFormate(wyString *query, wyBool ishtmlform);
	///initialze query format.
	/**
	@param query     :OUT formatting query	
	@param opts      :IN  SQl Formatter opts
	@return void.
	*/
	wyBool   StartQueryFormat(wyString *query, FormatOpts *opts, wyBool ishtmlform);
	///it will format the select uery
	/**
	@param query     :OUT formatting query	
	@param isexplain :IN  isexplain is used before select stmt.
	*/
	wyInt32  FormateSelectStmt(wyString *query, wyBool isexplain = wyFalse);

	///formatting the column list
	/**
	@param query		: OUT formatting query
	@param leadspaces	: IN  lead spaces to format query.
	@return void.
	*/
	wyInt32		FormateInsColList(wyString* query, wyInt32 leadspaces);

	///it will format the insert query
	/**
	@param query     :OUT formatting query	
	*/
	wyInt32		FormateInsStmt(wyString *query);

	///Checking whether ON DUPLICATE KEY UPDATE clause is present or not
	/**
	@param query     :OUT formatting query	
	*/
	void		IsUpdateOnDuplicate(wyString *query);

	///formatting the column Values
	/**
	@param query		: OUT formatting query
	@return void.
	*/
	wyInt32		FormateInsColValues(wyString *query);

	///it will format the insert query
	/**
	@param query		:OUT formatting query	
	@param leadspaces	:IN  lead spaces to format query.
	*/
	wyInt32		FormateInsertQuery(wyString *query, wyInt32 leadspaces);

	///formatting the Values clause
	/**
	@param query		: OUT formatting query
	@return void.
	*/
	void		FormateValuesClause(wyString* query);

	///formatting the column values in Values clause
	/**
	@param query		: OUT formatting query
	@return void.
	*/
	wyInt32		HandleValuesClauseForInsert(wyString* query, wyInt32 leadspaces);
	
	///formatting the selcted column list
	/**
	@param query     :OUT formatting query		
	@return wyTrue or wyFalse.
	*/
	wyInt32  ParseColumnLevel(wyString *query);

	//get column
	/**	
	@param query     :IN  rest query.	
	@param isfun     :IN  fun type column.
	*/
	void    GetColumn(wyString *column, wyBool isfun);	
	///get the subquery from the nmain query
	/**
	@param query     :OUT select main query	.
	return subquery.
	*/
	wyChar*  GetSubQuery(wyString * query);
	///matching the query for the columns
	/**
	@param query     :OUT select main query	.
	@returns formatted result,
	*/
	wyBool   MatchColumnPattern(wyString *query);	

	wyBool   SelectionLimitClauses(wyString *query);
	wyBool   FormateSelectionLimitClauses(wyString *query, wyString *clausename);
	void	 SetSelectionLimitPatterns(wyString *pattern, wyInt32 pattype);		
	wyBool   MatchSelectionLimitClauses(wyString *query, wyInt32 pattype);

	///handle column level subquery if non stack mode is selected for format.
	/**
	@param query            :OUT parsing query.
	*/
	wyBool   HandleColSubQueryInNonStackMode(wyString *query);
	///checking the query length .
	/**
	@param query            :OUT parsing query.
	*/
	wyBool   IsEmptyQuery(wyString *query, wyInt32 space);
	void     HandleCondExp(wyString *condexp, wyInt32 reslen, wyInt32 leadspaces); 
	///match function patterns
	/**
	@param query            :OUT parsing query.
	@return parsed result.
	*/
	wyInt32  IsFunAsColumn(wyString *query);
	//match the columnm aliases
	/**
	@param query            :OUT parsing query.
	@param isfrommatch      :OUT is from clause is used in sub query.
	@return parsed result.
	*/
	wyInt32  MatchColumnAliases(wyString *query, wyBool &isfrommatch);
	//checking the selected  column completion
	/**
	@param query            :OUT parsing query.
	@param isfrommatch      :OUT is from clause is used in sub query.
	@return parsed result.
	*/
	wyInt32  CheckColumnListCompletion(wyString *query, wyBool &isfrommatch);
	//formate the select query
	/**
	@param query            :OUT parsing query.
	@param parsecols        :IN parse columns 
	*/
	wyInt32  FormateSelectQuery(wyString *query, wyBool parsecols = wyFalse);
	//formate the query if all columns are selected.
	/**
	@param query            :OUT parsing query.
	@return wyTrue or wyFalse.
	*/
	wyBool   IsAllColsSelected(wyString *query);
	//tracing the cooment start pos.
	/**
	@param commstr          :IN query with comments.
	@param res              :IN rest query.
	@param uncommstr        :OUT query without comments.
	@return parsed result.
	*/
	wyBool   RemoveCommentsAndTracePOs(wyString *commstr, wyString *res, wyString *uncommstr);
	///removing the comments from the query
	/**
	@param  query         :OUT parsing query.
	@param  uncommstr     :OUT query with out comments.
	*/
	void     GetQueryWtOutComments(wyString *query, wyString *uncommstr);
	//matching the join clause condition(On or Using clause).
	/**
	@param  query         :OUT parsing query.
	@param  isjoinmatched :OUT is join clause matched.
	@return wyTrue or wyFalse.
	*/
	wyBool   MatchJoinCondClause(wyString *query);

	/// match IndexHint syntax
	/**
	@param  query         :OUT parsing query.
	@return void.
	*/
	void     HandleIndexHintSyntax(wyString *query);
	/// handle on clause
	/*
	@param  query         :OUT parsing query.	
	@return void.
	*/
	void     HandleOnClause(wyString *query);
	/// formate the sub query used in cond expressions
	/*
	@param  query         :OUT parsing query.
	@param  matchstr      :IN  matched string.
	@return void.
	*/
	void     FormateCondSubQuery(wyString *query, wyString *matchstr);
	///set the on cluase patterns
	/*
	@param  pattern       :OUT on clause pattern.
	@return void.
	*/
	void     SetOnClausePattern(wyString *pattern);	
	///formate the column list
	/*
	@param  pcollist      :IN column list buffer
	@param  selcolstr     :IN string for space reference
	@param  colcnt        :IN no of columns.
	@param  reslen        :IN rest string len.
	@return void.      
	*/
	void     FormateColumnList(ColumnList **pcollist, wyString *selcolstr, wyInt32 colcnt, wyInt32 reslen);
	/*fun used for adding line breaks after table and column*/
	/**
	@param  beforecomma   :IN comma position in query.
	@param  beforecomma   :IN space is used or not.
	@param  aftercomma    :IN space is used or not.
	@return void.
	*/
	void     LineBreaksWithComma(wyInt32 curquerypos, wyInt32 bfrecomma, wyInt32 aftercomma);
	//inserting the spaces to the formaating query
	/**
	@param  nospaces       :IN no of space to be insering to the query.
	@param  isnewline      :IN adding of new line to the formatting query .	
	@return void.
	*/
	void     InsertSpaces(wyInt32 nospaces, wyBool isnewline = wyFalse);
	///check all the patterrns which can use after table name.
	/**
	@param  query         :OUT parsing query.
	@param  indexsyntax   :IN  is match index syntax pattern.
	@return parse result.
	*/
	wyInt32  FormateSelTableReferences(wyString *query, wyBool indexsyntax);
	///formate the cond exp used in ON, where , group by , order by, having clauses 
	/**
	@param  query         :OUT parsing query.
	@param  leadspaces    :IN  lead spaces to format query.
	@return void.
	*/
	void     FormateCondExp(wyString *query, wyInt32 leadspaces);
	//here formate the query with in paranthesis
	/**
	@param  query         :OUT parsing query.
	@param  res           :OUT rest query.
	@param  leadspaces    :IN  lead spaces to format query.
	@return void.
	*/
	void     FormateCondStrInParanthesis(wyString *query, wyString *res, wyInt32 leadspaces);
	//new line last Occurrence position in string 
	/*
	@param str              :IN string in which  new line chracter is required.
	@return new line character pos.
	*/
	wyInt32  NewLineLastOccurrence(const wyChar *str);
	//handle the patterns matched in on clause
	wyInt32  PatternsMatchedInOnClause(wyString *query, wyString *res, wyString *condexp, 
									   wyInt32 leadspaces);

	///replacing multiple or single white spaces  with single horizantal space
	/**
	@param query            :OUT parsing query.		
	*/
	void     TrimSpaces(wyString *query);
	///handle the table level subquery
	/**
	@param query            :OUT parsing query.		
	@return void.
	*/
	void    HandleTableLevelSubquey(wyString *query, wyInt32 matchpatlen);		
	///formate the table joins.
	/**
	@param query            :OUT parsing query.	
	@param isjoinmatched    :OUT is another 
	*/
	wyBool   FormateTableJoins(wyString *query, wyBool matchjoin = wyTrue);
	///here subquery is retrieved from the main query and it is formatted by 
	//recursive calling Select Query formatter.
	/**	
	@param query            :OUT parsing query.	
	@return wyInt32.
	*/
	wyInt32  FormateSubQuery(wyString *query);
	///here checking table is joined with another table.
	/**
	@param query            :OUT parsing query.	
	@return wyTrue or wyFalse.
	*/
	wyBool   IsJoinTable(wyString *query);
	///formatting the subquery.
	/**
	@param query            :OUT parsing query.	
	@param leadspaces       : no of lead spaces to subquery
	*/
	wyBool   FormateIfSubQuery(wyString *query, wyInt32 leadspaces , wyBool wtnewline);
	///formatted the table if it is enclosed with in paranthesis
	/**
	@param query            :OUT parsing query.	
	@return void.
	*/
	void     TableEnclosedByParanthesis(wyString *query, wyInt32 leadspaces);
	/// match string pattern and get  substring
	/**
	@param subject          :OUT matching string
	@param pattern          :IN  matching pattern
	@param pcrecompileopts  :IN  compile options
	@param addsepchars      :IN is sep character is required in rest string .
	@param prevstr          :IN string upto match
	@returns regex return values.
	*/
	wyInt32  MatchStringAndGetResult(const wyChar*subject, wyChar *pattern, 
		         wyInt32 pcrecompileopts, wyString *res, wyBool addsepchars = wyFalse, wyBool prevstr = wyFalse, wyBool getsubstr = wyTrue);

	//get the substrings 	
	void     GetSubStrings(const wyChar*subject, wyString *res, wyInt32 *ovector, wyBool prevstr = wyTrue, 
				 wyBool addsepchars = wyFalse);	
	//freeing the memory allocated for columns
	/*
	@param   pcollist            :IN Selected column list.	
	@param   colcnt              :IN Selected column count.	
	@return void.
	*/
	void	 FreeColumnList(ColumnList **pcollist, wyInt32 colcnt);
	//If any options is added in select stmt ie,ALL,DISTINCt 
	/*
	@param query            :OUT parsing query.	
	@return void.
	*/
	void	 FormateSelectOptions(wyString *query);
	///formate the column if column is used as a subquery
	/*
	@param query            :OUT parsing query.	
	@return wyTrue or WyFalse.
	*/
	wyBool	 HandleColumnSubQuery(wyString *query );
	///formate the column if column is there with in paranthesis
	/*
	@param query            :OUT parsing query.	
	@return void.
	*/
	void	 ColumnEnclosedByParanthesis(wyString *query);	
	///this fun will restore comments to the formatted query
	/*
	@return void.
	*/
	void	 RestoreCommentsToQuery();	
	///it will return at which position comment is included in a formatted query
	/*
	@param 	cmntnumber : IN commnent count
	@return resultant comment pos
	*/
	wyInt32  GetCommentsInsertPos(wyInt32 cmntnumber);
	///matching the dumping option and table lock patterns.
	/*
	@param query            :OUT parsing query.
	@return wyTrue.
	*/
	wyBool   MatchDumpAndSelModePatterns(wyString *query);
	///trace the number of spaces trimmed or included before a specfic comment
	/*
	@param spacetrimpos : IN pos at which spaces are trimmed or included
	@param spaces       : IN trimmed or included spaces count
	*/
	void     TraceCommentPos(wyInt32 spacetrimpos, wyInt32 spaces);
	///here after trimming the extra spaces in query , we are adjusting the 
	///comment positions according to the trimmed lead spaces before comment
	void     AdjustCommentPosOnTrim();
	//if more tables are used in the table list with comma separation.
	/**
	@param query            :OUT parsing query.	
	@return int.
	*/
	wyInt32  TableListSepWithComma(wyString *query);
	///formatting the query if comma found
	/**
	@param bfrecomma  :IN is space is used before comma	
	@return int.
	*/
	wyInt32  CommaSeparator(wyString *query, wyInt32 bfrecomma);
	///formatting the from clause
	/**
	@param bfrefrom  :IN is space is used before from caluse
	@param afterfrom :IN is space is used afterfrom from caluse
	@param afterfrom :IN from keyword for keyword case.
	@return void.
	*/
	void     FormateFrom(wyInt32 querylen,  wyInt32 bfrefrom, wyInt32 afterfrom, wyChar *from);
	//it will give whether space character is used at findpos position in a given query or not
    /**
	@param findpos   : IN pos before which we are finding character type
	@return 1,  if space character is found.
	*/
	wyInt32  IsSpaceSepChar(wyInt32 findpos);
	///set pcre variables to default values on each string matching.	
	void     SetPcreVarsToDefault();
	///it will retrieve substring from the string
	/**
	@param  query    : IN string from which subquery is retreving
	@param  endpos   : IN ending position of substring
	@param  startpos : IN starting position of substring
	@return substr.
	*/
	wyChar*  GetSubStr(wyString *query, wyInt32 startpos, wyInt32 endpos);

	//it will add  spaces to mysql operators in a query
	/**
	@return wyTrue or wyFalse.
	*/
	wyBool   AddSpacesToOperator(wyString *query, wyChar *matchoperator, wyInt32 rtrimcnt);
	void    HandleCondExpPatterns(wyString *query, wyString *res, wyInt32 &leadspaces);

	void     SetCondStr(wyString *query, wyInt32 matchstart, wyInt32 *rtrimcnt);
	//in this fun we are checking subquery count exceeds three or not
	/*
	return wyTrue if exceeds.
	*/
	wyBool     IsSubQueryCountExceed();
	//if searching pattern is used inside (""", "`", "'") characters as a variable name 
	//or value qualifiers return true and retrieve type of  character 
	/**
	@param query    :IN parsing query.
	@param chartype :IN serching character.
	*/
	wyBool     IsPatternFindAsVarQualifier(wyString *query, wyString *chartype);

	//free the memory allocatted for comments
	void	   FreeCommentBuffer();

	void       SetFormatOptions(FormatOpts *opts);
	///it will format the update uery
	/**
	@param query     :OUT formatting query	
	*/
	wyInt32  FormateUpdateStmt(wyString *query);

	/// this function is used for checking whether SET clause is present or not
	/**
	@param query     :OUT formatting query	
	*/
	wyBool	HandleSetClause(wyString *query);
	
	///For handling table references
	/**
	@param query     :OUT formatting query	
	*/
	wyInt32	HandleTableReferences(wyString *query);

	///formatting the SET clause
	/**
	@param query     :OUT formatting query	
	*/
	wyInt32	FormateSetClause(wyString *query);
	
	///Handles the expressions in SET clause
	/**
	@param query     : OUT formatting query.
	@returns void,
	*/
	void	HandleColumnExpForUpdate(wyString *query);

	//handle the patterns matched in SET clause	
	/**
	@param query		: IN formatting query.
	@param res			: IN rest query.
	@param condexp		: IN cond expression
	@param leadspaces	: IN lead spaces to format query
	@returns formatted result,
	*/
	wyInt32 PatternMatchedInColumnExp(wyString *query, wyString *res, wyString *condexp, 
									   wyInt32 leadspaces);
	
	///Gets the column patterns with backticks
	/**
	@param pattern     :OUT column pattern.
	@returns void,
	*/
	void GetColumnPatternWithBacticks(wyString *pattern);

	///Gets the column patterns without backticks
	/**
	@param pattern     :OUT column pattern.
	@returns void,
	*/
	void GetColumnPatternWithOutBacticks(wyString *pattern);

	///it will format the delete uery
	/**
	@param query     :OUT formatting query	
	*/
	wyInt32		FormateDeleteStmt(wyString *query);

	///it will format the delete uery
	/**
	@param query     :OUT formatting query	
	*/
	wyInt32		FormateDeleteQuery(wyString *query);

	///Handles the FROM clause in DELETE stmt
	/**
	@param query     : OUT formatting query.
	@returns formatted result,
	*/
	wyInt32		HandleFromClause(wyString *query);

	///Handles the USING clause in DELETE stmt
	/**
	@param query     : OUT formatting query.
	@returns formatted result,
	*/
	wyInt32		HandleUsingClauseForDelete(wyString *query);

	void        InsertHtmlHorizantalSpace(wyInt32 nospaces);

	wyBool      IsCharsetUsedToSelString(wyString *query);	

	void         GetCollation(wyString *query, wyString *collation);

	static int  CharsetAndCollationCmp(const void *arg1, const void *arg2);

	void        FormateCollationSyntax(wyString *query);

	wyString    m_queryFormate;
	wyBool      m_parseerr;	
	FormatOpts  *m_opts;  	

	wyChar      m_newline[50];
	wyChar      m_horspace[10];	
//private:
	
	Comments    **m_pcmnts;
	wyInt32     m_cmntcnt;
	wyInt32     m_trackcmntno;	
	ColumnList  *m_collist;
	wyInt32     m_maxcollen;
	wyInt32     m_matchstartpos;
	wyInt32     m_matchendpos;	
	wyInt32     m_querylen;	
	wyString    m_uptomatch;		
	wyString    m_prevstr;	
	wyInt32     m_matchpatlen;		
	wyInt32     m_leadspaces;
	wyInt32     m_subquerycnt;
	//is query formatting  with comment
	wyBool      m_iscomment;
	wyString    m_spacerefquery;	
	wyBool	    m_ismultipletables;
	wyInt32     m_stmttype;
	wyBool		m_isusingpresent;
	wyBool      m_issort;
};
