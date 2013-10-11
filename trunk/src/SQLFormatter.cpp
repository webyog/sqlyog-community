/* Copyright (C) 2013 Webyog Inc

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

#include "SQLFormatter.h"
#include "FrameWindowHelper.h"

#define INDEX_HINT_SYNTAX     "^(USE|IGNORE|FORCE)\\s+(INDEX|KEY)(\\s+FOR\\s+JOIN)?\\("
#define COMMENTS              "(--[\\h|\\t]|#).*[\\n]?|--\\n|\\/\\*![\\d]|\\/\\*"
#define SUCCESS               1
#define FAILURE               2
#define PARSETABLE            3
#define NEWLINELENGTH		  2
#define SIZE_8K				 (8*1024)

wyChar *charsetlist[39] = {"big5", "dec8", "cp850", "hp8", "koi8r", "latin1", "latin2", "swe7", 
"ascii", "ujis", "sjis", "hebrew", "tis620", "euckr", "koi8u", "gb2312", "greek", 
"cp1250", "gbk", "latin5", "armscii8", "utf8", "ucs2", "cp866", "keybcs2", 
"macce", "macroman", "cp852", "latin7", "cp1251", "cp1256", "cp1257", "binary", "geostd8",
"cp932", "eucjpms", "utf16", "utf32", "utf8mb3"};


SQLFormatter::SQLFormatter()
{
	m_matchpatlen		= 0;
	m_parseerr			= wyFalse;
	m_collist			= NULL;
	m_maxcollen			= 0;
	m_leadspaces		= 0;
	m_cmntcnt			= 0;
	m_pcmnts			= NULL;
	m_trackcmntno		= 0;
	m_iscomment			= wyFalse;
	m_subquerycnt		= 0;
	m_ismultipletables	= wyFalse;
	m_stmttype			= SELECT_STMT;
	m_isusingpresent	= wyTrue;
	m_issort            = wyTrue;
}

SQLFormatter::~SQLFormatter()
{
}

void 
SQLFormatter::SetFormatOptions(FormatOpts *opts)
{
	 //Set formate Preference options
	opts->m_stacked = IsStacked();
	opts->m_linebreak = GetLineBreak();
	opts->m_colalign =  IsASAndAliasAlignment();	
	opts->m_spaceindentation = GetIndentation();	
}


wyBool
SQLFormatter::FormateTableJoins(wyString *query,  wyBool matchjoin)
{
	wyString pattern, res;	
	wyInt32  trimcnt, matchpatlen;
	wyInt32  spacetrimpos, nospaces;

	if(matchjoin == wyTrue && IsJoinTable(query) == wyFalse)
		return wyFalse;

	matchpatlen = m_matchpatlen;

	/*table is joining with another table by the  following ways
	1) subquery as a table like ( select * from table) as alias
	2) tables inside brackets like  ( table)
	3) direct way like tablename
	*/
	query->LTrim(&trimcnt);
	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	nospaces = 1 - trimcnt;
	//tracing the comment pos after join caluse
	TraceCommentPos(spacetrimpos, nospaces);

	//if sub query is used as a joining table.
	if(query->GetCharAt(0) == '(')
		HandleTableLevelSubquey(query, matchpatlen);		
	else
	{
		MatchTablePattern(query);

		if(m_parseerr == wyTrue)
			return wyTrue;		

		//adding table name to the query
		m_queryFormate.AddSprintf("%s", m_prevstr.GetString());	

        //match join cond clause(on or using)
		if(MatchJoinCondClause(query) == wyTrue || m_parseerr == wyTrue)
			return wyTrue;

		//if comma is found return wyTrue. we are processing it later
		if(query->GetCharAt(0) == ',')
			return wyTrue;
		
		//if it is update stmt then we are searching for SET clause
		if(m_stmttype == UPDATE_STMT)
		{
			if(HandleSetClause(query) == wyTrue || m_parseerr == wyTrue)
				return wyTrue;
		}
		//if it is not update,Select ,we are searching for where,order by  group by etc.
		else if(SelectionLimitClauses(query) == wyTrue || m_parseerr == wyTrue)
			return wyTrue;

		//if query is like "select * from table1 inner join table2 inner join table3"
		if(FormateTableJoins(query) == wyTrue)
			return wyTrue;

		HandleIndexHintSyntax(query);

		if(m_stmttype == SELECT_STMT)
		{	
			//if it is select stmt then only we are searching for procedure , outfile ...
			if(MatchDumpAndSelModePatterns(query) == wyTrue)
				return wyTrue;
		}

		MatchTableAliasPattern(query);
		HandleIndexHintSyntax(query);
	}

	if(m_parseerr == wyTrue)
		return wyTrue;	

	/*if on cond is used for join clauses we no need to match any one of following
	patterns becuase we matched all these patterns in that fun only.*/
	if(query->GetCharAt(0) == ',')
		return wyTrue;

	if(MatchJoinCondClause(query) == wyTrue || m_parseerr == wyTrue)
		return wyTrue;

	//if it is update stmt then we are searching for SET clause
	if(m_stmttype == UPDATE_STMT)
	{
		if(HandleSetClause(query) == wyTrue || m_parseerr == wyTrue)
			return wyTrue;
	}
	//if it is not update,ie, Select,we are searching for where,order by  group by etc.
	else if(SelectionLimitClauses(query) == wyTrue || m_parseerr == wyTrue)
		return wyTrue;

	FormateTableJoins(query);
	return wyTrue;
}

/* query 
having Index_Hint syntax only if  table is joining with any single table
otherwise query doesn`t contain these  syntax.
derived tables also doesnt have Index_Hint syntax.*/
void
SQLFormatter::HandleIndexHintSyntax(wyString *query)
{
	wyString  pattern, res, temp;
	wyInt32   matchret = 0;
	wyInt32   len = 0, trimcnt;
	wyInt32   spacetrimpos, nospaces;
	
	query->LTrim(&trimcnt);

	if(IsEmptyQuery(query, trimcnt) == wyTrue)
			return;	

	pattern.SetAs(INDEX_HINT_SYNTAX);
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		       PCRE_COMPLIE_OPTIONS, &res);

	if(matchret == NOTMATCHED)
		return ;

	len = GetStrLenWithInParanthesis(res.GetString(), wyTrue);
	
	//if corresponding paranthesis is not found 
	if(len == -1)
	{
		m_parseerr = wyTrue;
		return ;
	}

	m_queryFormate.AddSprintf("%s%s", m_horspace, GetSubStr(query, 0, m_matchendpos + len + 1));

	trimcnt = IsSpaceSepChar(query->GetLength());
	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	nospaces = 1 - trimcnt;
	TraceCommentPos(spacetrimpos, nospaces);
	//if you want to erase few characters, better to use erase fun instead of substr 
	res.Erase(0, len + 1);
	query->SetAs(res.GetString());
}

//matching the join condition clause(On or Using clause).
wyBool
SQLFormatter::MatchJoinCondClause(wyString *query)
{
	wyString pattern, res;
	wyInt32  matchret, len, space, trimcnt;	
	wyString condclause;
	wyInt32  spacetrimpos, nospaces;
		
	query->LTrim(&trimcnt);

	if(IsEmptyQuery(query, trimcnt) == wyTrue)
		return wyTrue;	
	
	pattern.SetAs(JOINCOND_CLAUSE);    
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		       PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);	

	if(matchret == NOTMATCHED)		
		return wyFalse;	

	//depends upon fun flow  some times query will come with trim or  with out trim
	// so the result we got from above trim will not tell exact spaces that exist
	//at this pos , but we need exact space presency in a query at this pos , for that 
	//we used following fun ,it  will give the exact space occurence
	space = IsSpaceSepChar(query->GetLength());
	
	//lead spaces before on is 11(9 +2)=(joinclause leadspaces + two addional spaces)

	spacetrimpos = m_querylen - query->GetLength() - space;
	nospaces =  m_leadspaces + m_opts->m_spaceindentation + ONCLAUSELS + NEWLINELENGTH  - space;
	TraceCommentPos(spacetrimpos, nospaces);	
	//here one space in comment trace is for new line character
	condclause.Sprintf("%s", GetSubStr(query, 0, m_matchpatlen - 1));	
	InsertSpaces(m_leadspaces + m_opts->m_spaceindentation + ONCLAUSELS, wyTrue);
	m_queryFormate.AddSprintf("%s%s", condclause.GetString(), m_horspace); 
	
	query->SetAs(res);
	query->LTrim(&trimcnt);
	//add space after on clause		
	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	TraceCommentPos(spacetrimpos, 1 - trimcnt);	
    
	//on condition used in joins
	if(m_matchpatlen == 3)
	{
		HandleOnClause(query);
		return wyTrue;
	}
	
	/*Formation of USing Clause
	1) look for next paranthesis 
	2)add the string between open and close paranthesis to the formatted query as it is.
	*/		

	if(query->GetCharAt(0) == '(')
		query->Erase(0, 1);

	len = GetStrLenWithInParanthesis(query->GetString(), wyTrue);

	//if corresponding paranthesis is not found
	if(len == -1)
	{
		m_parseerr = wyTrue;
		return wyTrue;		
	}

	m_queryFormate.AddSprintf("(%s", GetSubStr(query, 0, len + 1));
	query->Sprintf("%s", GetSubStr(query, len+1, query->GetLength() - len));
	query->LTrim(&trimcnt);

	if(IsEmptyQuery(query, trimcnt) == wyTrue)
		return wyTrue;	

	return wyFalse;
}

/*here formatting the on clause condition expression and also
 handled all the patterns which comes after on clause*/
void
SQLFormatter::HandleOnClause(wyString *query)
{
	wyString pattern, res;
	wyInt32  pos;
	wyString tempstr, substr;
	wyInt32  matchret;		
	wyInt32  leadspaces;		
	wyString condexp, searchchar;
	wyString str;

	SetOnClausePattern(&pattern);	

	while(1)
	{
		//leadspace  on clause + standard space indentation and two spaces for 'AND'
		// or "OR"
		leadspaces = m_leadspaces + m_opts->m_spaceindentation + ONCLAUSELS + 2;

		if(m_parseerr == wyTrue)
			return;		
		
		if(query->GetLength() == ZERO_LENGTH)
		{
			condexp.Add(query->GetString());
			HandleCondExp(&condexp, 0, leadspaces);
			return ;
		}

		matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
			PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

		if(matchret == NOTMATCHED)
		{
			condexp.Add(query->GetString());
			HandleCondExp(&condexp, 0, leadspaces);
			query->SetAs("");
			return;
		}

		str.SetAs(GetSubStr(query, 0, m_matchendpos - 1));
		//following fun will give whether matched pattern is used inside backticks or not.
		//if pattern is used inside backticks,  skip all characters up to next backtick and 
		//continue parsing with the rest string.		
		if(IsPatternFindAsVarQualifier(&str, &searchchar) == wyTrue)
		{
			if((pos = res.Find(searchchar.GetString(), 0)) == -1)
			{
				m_parseerr = wyTrue;
				return;
			}

			condexp.AddSprintf("%s", GetSubStr(query, 0, m_matchendpos + pos));
			//here erasing characters up to next backticks
			res.Erase(0, pos + 1);
			query->SetAs(res);
			continue;
		}

		if(PatternsMatchedInOnClause(query, &res, &condexp, leadspaces) == SUCCESS)
			return;	
	}  	
}

/* here we need to match only patterns which is required to match
	on condition , instead of that we have included few more 
	patterns which comes afterr on clause(see in the pattern list).
	because we can avoid matching of "ON" condition patterns other than in on conditions 
	only if we include all other patterns which comes after "ON" condition.
	example :suppose after ON condition if we have where clause 
	and  if we have not included  where pattern, match will continue after where caluse also.*/	
wyInt32
SQLFormatter::PatternsMatchedInOnClause(wyString *query, wyString *res, wyString *condexp, 
									   wyInt32 leadspaces)
{
	wyInt32  len, trimcnt, pos;
	wyString tempstr;
	wyInt32  spacetrimpos, nospaces;

	// m_matchpatlen will give to which pattern the query is matched.
	if(m_matchpatlen == 1)
	{
		//if comma is found in  parsing
		condexp->AddSprintf("%s", GetSubStr(query, 0, m_matchstartpos));
		HandleCondExp(condexp, res->GetLength(), leadspaces);
		query->SetAs(res->GetString());			
	}
	else
	{
		tempstr.Sprintf("%s", GetSubStr(query, m_matchstartpos, m_matchpatlen -1));		

		//if subquery is used in cond exp,save this subquery to condexp string
		// and continue parsing with the rest string untill on cond exp is completed
		if(tempstr.GetCharAt(0) == '(')
		{
			res->Erase(0, 1);
			len = GetStrLenWithInParanthesis(res->GetString(), wyTrue);
			
			//if corresponding paranthesis is not found.
			if(len == -1)
			{
				m_parseerr = wyTrue;
				return SUCCESS;
			}
		
			condexp->AddSprintf("%s", GetSubStr(query, 0, m_matchendpos + len + 1));
			query->SetAs(GetSubStr(res, len + 1, res->GetLength() - len - 1));			
			return CONTINUE_PARSE;
		}		
	
		condexp->AddSprintf("%s", GetSubStr(query, 0, m_matchstartpos));        
		HandleCondExp(condexp, res->GetLength()+ m_matchpatlen - 1 , leadspaces);
		query->SetAs(*res);

		//space pos before selection limit clauses.
		pos = query->GetLength() +  tempstr.GetLength();		
		trimcnt = IsSpaceSepChar(pos);

		//if join clause is again used in a query we have to handle it by recursive  calling 
		//of join  fun		
		if(tempstr.FindIWithReverse("JOIN", 0, wyTrue) == 0)
		{
			InsertSpaces(m_leadspaces + m_opts->m_spaceindentation, wyTrue);							
			spacetrimpos = m_querylen - query->GetLength()- trimcnt;
			nospaces = m_leadspaces + m_opts->m_spaceindentation + NEWLINELENGTH;
			TraceCommentPos(spacetrimpos, nospaces);
			m_queryFormate.AddSprintf("%s%s", tempstr.GetString(), m_horspace);				
			FormateTableJoins(query, wyFalse);
			return SUCCESS;
		}

		//trace comment pos before selection limit caluses.		
		//in cond exp we have trimmed one space hence here we need to add one space always
		spacetrimpos = m_querylen - pos - trimcnt;
		nospaces = m_leadspaces + NEWLINELENGTH;
		TraceCommentPos(spacetrimpos, nospaces);
		if(tempstr.FindI("SET") == 0)//if it is update statement then SET clause
		{
			InsertSpaces(m_leadspaces, wyTrue);
			m_queryFormate.AddSprintf("%s", tempstr.GetString());
			InsertSpaces(m_leadspaces + SPACEAFTERKW);
			TraceCommentPos(spacetrimpos, SPACEAFTERKW);
            FormateSetClause(query);
		}
		else
			FormateSelectionLimitClauses(query, &tempstr);

		query->SetAs("");
	}

	return SUCCESS;
}

/*
selection limit patterns
1) where 2)group by 3) order by 4)having 5)limit
.match the string to above patterns and formate the query according to that patterns.
*/
wyBool
SQLFormatter::SelectionLimitClauses(wyString *query)
{
	wyString  res;
	wyInt32   matchret, space;
	wyString  pattern;
	wyInt32   spacetrimpos, nospaces;
	wyString  tempstr, condexp, condclause;	

	query->LTrim(&space);

	if(IsEmptyQuery(query, space) == wyTrue)
		return wyTrue;	
		
	if(m_stmttype == DELETE_STMT)
	{
		pattern.SetAs("^WHERE[`\\(\\s]");
		if(m_ismultipletables == wyFalse)//if mutipletables then order by,limit will not come in update stmt
			pattern.Add("|^ORDER\\s+BY[`\\)\\s]|^LIMIT\\s");
	}
	else
	        pattern.SetAs(SELECT_LMTCLAUSES);
	
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED || res.GetLength() == 0)
		return wyFalse;

	//here before limit clause we are adding m_leadspaces and new line 
	space = IsSpaceSepChar(query->GetLength());
	spacetrimpos = m_querylen - query->GetLength()- space;
	nospaces = m_leadspaces + NEWLINELENGTH - space;
	TraceCommentPos(spacetrimpos, nospaces);
	condclause.Sprintf("%s", GetSubStr(query, 0, m_matchpatlen - 1));
	FormateSelectionLimitClauses(&res, &condclause);
	query->SetAs("");
	return wyTrue;
}

void
SQLFormatter::SetSelectionLimitPatterns(wyString *pattern, wyInt32 pattype)
{
	switch(pattype)
	{	
	case GROUPBY:
		pattern->SetAs(GROUPBY_CLAUSE);
		break;
	case HAVING:
		pattern->SetAs(HAVING_CLAUSE);
		break;	
	case ORDERBY:
		pattern->SetAs(ORDERBY_CLAUSE);
		break;			
	case LIMIT:
		pattern->SetAs(LIMIT_CLAUSE);    
		break;		
	}	
}

wyBool
SQLFormatter::StartQueryFormat(wyString *query, FormatOpts *opts, wyBool ishtmlform)
{
	wyString formatequery, temp;

	if(!query)
		return wyFalse;
		
	strcpy(m_newline,  "\r\n");	
	strcpy(m_horspace, " ");	

	if(query->GetLength() > SIZE_8K)
		return wyFalse;

	formatequery.SetAs(*query);
	m_opts = opts;	
	//formatting the query
	return DoDMSSqlFormate(&formatequery, ishtmlform);		
}

/*steps
1)seraching for subquery and  pattaerns which  can use to restrict the selection result
2)retriving the cond exp of a clause in a query.
3)if subquery is found in cond exp retrieving it as a part of cond exp.
4)format  cond exp before formatting other patterns.
*/
wyBool
SQLFormatter::MatchSelectionLimitClauses(wyString *query, wyInt32 pattype)
{
	wyString  res;
	wyInt32   matchret, condexplen, len;
	wyInt32   space, querylen, reslen;
	wyString  pattern;
	wyString  tempstr, condexp;
	wyInt32   spacetrimpos, nospaces;

	SetSelectionLimitPatterns(&pattern, pattype);
	condexp.SetAs("");
	querylen = query->GetLength();	

	while(1)
	{
		matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
			PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

		//here getting cond string by parsing all the pattern which comes after cond clause
		if(matchret == NOTMATCHED)
		{
			condexp.Add(query->GetString());
			HandleCondExp(&condexp, 0, m_leadspaces + m_opts->m_spaceindentation + 2);
			return wyFalse;
		}
		
		if(m_prevstr.FindIWithReverse("SELECT", 0, wyTrue) == 0)
		{
			res.Erase(0, 1);
			len = GetStrLenWithInParanthesis(res.GetString(), wyTrue);

			if(len == -1)
			{			
				m_parseerr = wyTrue;
				return wyTrue;
			}

			condexp.AddSprintf("%s", GetSubStr(query, 0, m_matchstartpos + m_matchpatlen + len + 1));
			query->Sprintf("%s", GetSubStr(&res, len + 1, res.GetLength() - len - 1));		
			continue;
		}	

		condexplen = query->GetLength() -(res.GetLength() + m_matchpatlen - 1);
		condexp.AddSprintf("%s", GetSubStr(query, 0, condexplen));
		tempstr.Sprintf("%s", GetSubStr(query, m_matchstartpos, m_matchpatlen - 1));
		
		reslen = querylen - condexp.GetLength();
		HandleCondExp(&condexp, reslen, m_leadspaces + m_opts->m_spaceindentation + 2);

		space = IsSpaceSepChar(querylen);
		spacetrimpos = m_querylen - querylen - space;
		nospaces = m_leadspaces + NEWLINELENGTH;
		TraceCommentPos(spacetrimpos, nospaces);
		//tempstr contain clause to which clause pattern is matched
		FormateSelectionLimitClauses(&res, &tempstr);

		return wyTrue;
	}
}

wyBool
SQLFormatter::FormateSelectionLimitClauses(wyString *query, wyString *clausename)
{	
	wyInt32 trimcnt, clausetype;	
	wyInt32 spacetrimpos, nospaces;
	wyInt32	leadspaces;
	
	InsertSpaces(m_leadspaces, wyTrue);
	
	query->LTrim(&trimcnt);
	//adding selection limit clauses to the formatting query
	m_queryFormate.AddSprintf("%s", clausename->GetString());
	InsertSpaces(SPACEAFTERKW);

	if(clausename->FindI("WHERE") == 0)
	{
		//if it is update or delete stmt , then there is no group by and having clauses.
		if(m_stmttype == UPDATE_STMT || m_stmttype == DELETE_STMT)
		{   
			//if it is single table syntax, then only we are searching for order by & limit
			if(m_ismultipletables == wyFalse)
				clausetype = ORDERBY;
		else
			{	
				//if it is multiple table syntax, then there is no order by & limit, the remaining string will take as conditional expression
				leadspaces = m_leadspaces + m_opts->m_spaceindentation + 2;
				TraceCommentPos(m_querylen - query->GetLength(), SPACEAFTERKW - trimcnt);
				HandleCondExp(query, 0, leadspaces);
				return wyTrue;
			}
		}
		else
			clausetype = GROUPBY;
	}
	else if(clausename->FindI("GROUP") == 0)
		clausetype = HAVING;	
	else if(clausename->FindI("HAVING") == 0)
		clausetype = ORDERBY;	
	else if(clausename->FindI("ORDER") == 0)
		clausetype = LIMIT;	
	else if(clausename->FindI("LIMIT") == 0)
	{
		spacetrimpos = m_querylen - query->GetLength() - trimcnt;
		nospaces = SPACEAFTERKW - trimcnt;
		TraceCommentPos(spacetrimpos, nospaces);
		//here limit cluase we are adding as it is with out changes
		m_queryFormate.AddSprintf("%s", query->GetString());		
		return wyTrue;
	}
	else
	{
		m_parseerr = wyTrue;
		return wyTrue;
	}

	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	nospaces = SPACEAFTERKW - trimcnt;
	TraceCommentPos(spacetrimpos, nospaces);
	MatchSelectionLimitClauses(query, clausetype);

	return wyTrue;
}

//on clause patterns with all other patterns (clauses having subqueries) comes after on condition.
void
SQLFormatter::SetOnClausePattern(wyString *pattern)
{
	wyString selpattern;

	pattern->SetAs(",|((INNER\\s+|CROSS\\s+)?JOIN|STRAIGHT_JOIN|");
	pattern->Add("(LEFT|RIGHT)(\\s+OUTER)?(\\s+JOIN)|");
	pattern->Add("(LEFT|RIGHT)(\\s+OUTER)?(\\s+JOIN)|");
	pattern->Add("NATURAL((\\s+LEFT|\\s+RIGHT)(\\s+OUTER)?)?\\s+JOIN");
	
	//if it is update stmt then we are searching for SET clause
	if(m_stmttype == UPDATE_STMT)
		pattern->Add("|SET)[`\\(\\s]");
	else
	{
		pattern->Add("|WHERE");		
	
		if(m_stmttype == SELECT_STMT)//if it is select stmt , then group by and having is present
		{
			pattern->Add("|GROUP\\s+BY");
			pattern->Add("|HAVING");
		}

		pattern->Add("|ORDER\\s+BY)[`\\(\\s]");//for delete and select stmt where , order by and limit is present
		pattern->Add("|LIMIT\\s");    
	}
	pattern->Add("|\\(\\s*SELECT");	
	
}

void
SQLFormatter::HandleCondExp(wyString *condexp, wyInt32 reslen, wyInt32 leadspaces)
{
	wyInt32 querylen;

	//here we are passing only the conditional expression, so we need to set length of the query  
	querylen = m_querylen;
	m_querylen = m_querylen - reslen;
	FormateCondExp(condexp, leadspaces);
	//set original length of the query
	m_querylen = querylen;

}

//adding one space before operator and after the operator
wyBool
SQLFormatter::AddSpacesToOperator(wyString *query,  wyChar *matchedope, wyInt32 rtrimcnt)
{
	wyInt32  spacetrimpos;	

	//trace spaces before the operator	
	spacetrimpos = m_querylen - (query->GetLength() - (m_matchstartpos - 1));
	TraceCommentPos(spacetrimpos, 1 - rtrimcnt);	

	m_queryFormate.Add(m_horspace);
	m_queryFormate.AddSprintf("%s%s", matchedope, m_horspace);	

	//trace spaces after the operator	
	if(query->GetCharAt(m_matchendpos) != ' ')
	{
		spacetrimpos = m_querylen - (query->GetLength() - m_matchendpos - 1);
		TraceCommentPos(spacetrimpos, 1);		
	}

	return wyTrue;
}

/* in on clause pattern  we are looking for open bracket if it matches taking 
the entire string untill next corresponding brackets and passed that string to the following fun.
in this function we are also  matching 'and' 'or',operators,  'subquery' patterns 
*/
void
SQLFormatter::FormateCondExp(wyString *query, wyInt32 leadspaces)
{
	wyInt32  matchret, pos, templspaces;
	wyString pattern, res; 	
	wyInt32  trimcnt;
	wyInt32  spacetrimpos;
	wyString temp, searchstr;

	templspaces = leadspaces;
	//if "and", "or" keywords are there align the keywords 
	//AND, OR type formation will do  by adding new line + prev len (after on) the query 
	SetSelectPattern(&pattern, CONDSUB_QUERY);
	pattern.Add("|\\(|([\\s`'\"]|^)(AND|OR|XOR)[\\s\\(`'\"]");
	//set operators pattern	for adding one space around the operator.
	pattern.AddSprintf("|(%s)", MYSQL_OPERATORS);

	while(1)
	{
		leadspaces = templspaces;
		
		if(query->GetLength() == 0 || m_parseerr == wyTrue)
			return;		

		matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
				PCRE_COMPLIE_OPTIONS, &res, wyTrue);
		
		if(matchret == NOTMATCHED)
		{
			query->LTrim(&trimcnt);
			spacetrimpos = m_querylen -  query->GetLength() - trimcnt;
			TraceCommentPos(spacetrimpos, -trimcnt);			
			query->RTrim(&trimcnt);		
			spacetrimpos = m_querylen - trimcnt;
			TraceCommentPos(spacetrimpos, -trimcnt);
			m_queryFormate.Add(query->GetString());
			return;
		}

		temp.SetAs(GetSubStr(query, 0, m_matchendpos - 1));
		//following fun will give whether matched pattern is used inside backticks or not.
		//if pattern is used inside backticks skip all characters up to next backtick and 
		//continue parsing with the rest string.		
		if(IsPatternFindAsVarQualifier(&temp, &searchstr) == wyTrue)
		{
			if((pos = res.Find(searchstr.GetString(), 0)) == -1)
			{
				m_parseerr = wyTrue;
				return ;
			}

			leadspaces += m_matchstartpos + pos;
			m_queryFormate.AddSprintf("%s", GetSubStr(query, 0, m_matchendpos + pos));
			res.Erase(0, pos +1);
			query->SetAs(res);		
			continue;
		}

		HandleCondExpPatterns(query, &res, leadspaces);			
	}
}

void
SQLFormatter::HandleCondExpPatterns(wyString *query, wyString *res, wyInt32 &leadspaces)
{
	wyInt32   rtrimcnt = 0;
	wyString  selkw, matchpat;
	wyInt32   spacetrimpos, nospaces, matchpatlen;
	wyInt32	  matchstart, trimcnt;
	wyChar    sepchar;

	matchpat.SetAs(GetSubStr(query, m_matchstartpos, m_matchpatlen));		

	if(matchpat.FindI("(", 0) == 0)
	{
		SetCondStr(query, m_matchstartpos, &rtrimcnt);
		//open paranthesis.
		spacetrimpos = m_querylen - res->GetLength() - rtrimcnt;
		TraceCommentPos(spacetrimpos, -rtrimcnt);
		leadspaces = m_queryFormate.GetLength() - NewLineLastOccurrence(m_queryFormate.GetString()) - 1;
		FormateCondStrInParanthesis(query, res, leadspaces);    
	}
	//if subquery is used in cond exp
	else if(matchpat.FindI("SELECT", 0) == 0)
	{
		SetCondStr(query, m_matchstartpos, &rtrimcnt);
		selkw.Sprintf("%s", GetSubStr(query, m_matchstartpos, m_matchpatlen - 1));			
		query->SetAs(*res);
		
		if(m_queryFormate.GetCharAt(m_queryFormate.GetLength() - 1) != '(')
		{
			spacetrimpos = m_querylen - (query->GetLength() - (m_matchstartpos -1));
			TraceCommentPos(spacetrimpos, 1);
			m_queryFormate.Add(m_horspace);
		}
		//here this function  will formate the condional  subquery
		FormateCondSubQuery(query, &selkw);	
	}
	//except one operator remaining patterns length are less than three
	else if(m_matchpatlen >= 3 && (matchpat.FindI("<=>", 0) != 0))
	{
		sepchar = matchpat.GetCharAt(0);
		matchstart = m_matchstartpos;
		matchpatlen = m_matchpatlen;

		//sep characters to the keyword is used to avoid matching of  variable name ends with keywords 
		if(sepchar == '"' || sepchar == '\'' || isspace(sepchar) != 0)
		{
			matchstart += 1;
			matchpatlen -= 1;
			//if 'OR"' condition is used we are giving one lead space before 'OR'
			//when compared to 'AND'			
			leadspaces += ((m_matchpatlen == 5)?0:1);
		}		
		else		 
			leadspaces += ((m_matchpatlen == 4)?0:1);	

		SetCondStr(query, matchstart, &rtrimcnt);
		spacetrimpos = m_querylen - query->GetLength() + matchstart - rtrimcnt;
		nospaces = leadspaces + NEWLINELENGTH - rtrimcnt;
		TraceCommentPos(spacetrimpos, nospaces);
		InsertSpaces(leadspaces, wyTrue);					
		res->LTrim(&trimcnt);
		m_queryFormate.AddSprintf("%s%s",GetSubStr(query, matchstart, matchpatlen -1), m_horspace);
		//if space is not exist  before AND , OR keyword we are adding one lead spaces to comment 
		spacetrimpos = m_querylen - res->GetLength() -  trimcnt;
		TraceCommentPos(spacetrimpos, 1 - trimcnt);

		//if AND, XOR, OR patterns matches with open paranthesis
		if(res->GetCharAt(0) == '(')
		{
			leadspaces += matchpatlen;
			FormateCondStrInParanthesis(query, res, leadspaces);	
		}
		else
			query->SetAs(*res);
	}
	else
	{
		SetCondStr(query, m_matchstartpos, &rtrimcnt);
		//if matched patterrn is other than mysql operator patterns
		AddSpacesToOperator(query, (wyChar*)matchpat.GetString(),  rtrimcnt);
		//erasing the operator and then continuing the query parsing with rest string
		res->Erase(0, 1);
		res->LTrim();
		query->SetAs(*res);	
	}	
}

//get the string upto match and set it to the formatted query
void
SQLFormatter::SetCondStr(wyString *query, wyInt32 matchstartpos, wyInt32 *rtrimcnt)
{
	wyInt32   spacetrimpos, ltrimcnt =0;
	wyString  substr;

	//it will trim extra spaces in cond query	
	substr.Sprintf("%s", GetSubStr(query, 0, matchstartpos));	
	substr.LTrim(&ltrimcnt);
	spacetrimpos = m_querylen - query->GetLength();
	TraceCommentPos(spacetrimpos, -ltrimcnt);
	substr.RTrim(rtrimcnt);
	m_queryFormate.Add(substr.GetString());
}

//function used for finding last new line  accourence in a string
wyInt32
SQLFormatter::NewLineLastOccurrence(const wyChar *str)
{	
	wyString searchstr;
	wyInt32  newlinepos, lastoccurence;	

	searchstr.SetAs(str);

	newlinepos = searchstr.FindIWithReverse(m_newline, 0, wyTrue);

	if(newlinepos == -1)
		return 0;

	lastoccurence = searchstr.GetLength() - newlinepos - 1;

	return lastoccurence;	
}

/*this function will retrieve string inside paranthesis and formatting
condition expressions by recursive calling cond exp fun.*/
void
SQLFormatter::FormateCondStrInParanthesis(wyString *query, wyString *res, wyInt32 leadspaces)
{ 
	wyString temp;
	wyInt32 pos, reslen;	
	
	m_queryFormate.Add("(");
	leadspaces += 1;

	//erasing the open paranthesis
	res->Erase(0, 1);
    pos = GetStrLenWithInParanthesis(res->GetString(), wyTrue);	
	
	if(pos == -1)
	{
		m_parseerr = wyTrue;
		return;	
	}

	//subquery with in paranthesis
	temp.SetAs(GetSubStr(res, 0, pos));
    reslen = res->GetLength() - temp.GetLength();	
	HandleCondExp(&temp, reslen, leadspaces);
	m_queryFormate.Add(")");
	//query after subquery
	query->SetAs(GetSubStr(res, pos + 1, res->GetLength() - pos - 1));		
}

/*this fun is used when tables are used inside paranthesis like select * from (table, table2).
*/
void
SQLFormatter::TableEnclosedByParanthesis(wyString * query, wyInt32 leadspaces)
{
	wyString subquery, spacerefquery;
	wyInt32  len;
	wyChar   *temp;
	wyInt32  tleadspaces, querylen;
	wyInt32	 trimcnt;
	wyInt32	 spacetrimpos;
    
	//erasing the open paranthesis
	query->Erase(0, 1);
	m_queryFormate.Add("(");

	query->LTrim(&trimcnt);
	spacetrimpos = m_querylen - query->GetLength();
	TraceCommentPos(spacetrimpos, -trimcnt);

	len = GetStrLenWithInParanthesis(query->GetString(), wyTrue);

	if(len == -1)
	{
		m_parseerr = wyTrue;
		return;
	}

	subquery.Sprintf("%s", GetSubStr(query, 0, len)); 
	len = subquery.GetLength();
	tleadspaces = m_leadspaces;
	m_leadspaces +=  1 + leadspaces;

	querylen = m_querylen;
	m_querylen = m_querylen - (query->GetLength() - len); 

	FormateSelectQuery(&subquery);	

	m_queryFormate.Add(")");
	m_querylen = querylen;
	m_leadspaces = tleadspaces;

	//query after subquery
	temp = GetSubStr(query, len + 1, (query->GetLength() - len - 1));
	query->Sprintf("%s",  temp);    
}

//get the subquery from main query
wyChar*
SQLFormatter::GetSubQuery(wyString * query)
{
	wyInt32 len;

	len = GetStrLenWithInParanthesis(query->GetString(), wyTrue);

	if(len == -1)
		return NULL;

	return(query->Substr(0, len));  

}

//fun will give whether table is joined with another table or not
wyBool 
SQLFormatter::IsJoinTable(wyString *query)
{
	wyString pattern, res;
	wyInt32  matchret, space;
	wyInt32  spacetrimpos, nospaces;

	//the folowing pattern is  used to match the string when it start with joins and ends with  
	//separating characters. here the use of adding separate characters at the end is for avoiding
	//the matching of following formate sequences inside the tables like `tablejoin`.

	pattern.SetAs("^((INNER\\s+|CROSS\\s+)?JOIN|STRAIGHT_JOIN|");
	pattern.Add("(LEFT|RIGHT)(\\s+OUTER)?(\\s+JOIN)|");
	pattern.Add("NATURAL((\\s+LEFT|\\s+RIGHT)(\\s+OUTER)?)?\\s+JOIN)[`|\\(|\\s]");

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);	

	if(matchret == MATCHED)
	{
		space = IsSpaceSepChar(query->GetLength());
		InsertSpaces(m_leadspaces + m_opts->m_spaceindentation, wyTrue);	
		//here we are adding (m_leadspaces + m_opts->m_spaceindentation+1) no of spaces
		//before join clause . so we have to add thses lead spaces to comments
        spacetrimpos = m_querylen - query->GetLength() - space;
		nospaces = m_leadspaces + m_opts->m_spaceindentation + NEWLINELENGTH - space;
		TraceCommentPos(spacetrimpos, nospaces);
		//erasing of sep characters
		m_prevstr.Erase(m_prevstr.GetLength() - 1, 1);
		m_queryFormate.AddSprintf("%s%s", m_prevstr.GetString(), m_horspace);		
		m_ismultipletables = wyTrue;
		query->SetAs(res);
		return wyTrue;
	}

	return wyFalse;
}

//function will parse the diff types of table occurences in a query
void
SQLFormatter::MatchTablePattern(wyString *query)
{
	wyString  res;
	wyString  pattern;
	wyInt32   matchret;
	wyString  temp;

	//following pattern is used to match the table if it comes any one of the following formate
	// 1) `db`.`table` 2) `db`.table 3) `table`
	pattern.SetAs(TABLEPAT_WTBKQUOTES);   	

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyFalse,wyTrue);
	
	if(matchret == MATCHED)
	{		
		query->SetAs(res);
        return;		
	}

	//here one back quote is additionally adding to subjected query for easy parsing purpose.
	//this procedure is doing only when db is used with out backquotes in a query(db.table or db.`table`).
	temp.Sprintf("`%s", query->GetString());	

	if(m_stmttype == INSERT_STMT)
		pattern.SetAs(TABLEPAT_WTOUTBKQUOTES_INS);
	else
		pattern.SetAs(TABLEPAT_WTOUTBKQUOTES);
	
	matchret = MatchStringAndGetResult(temp.GetString(), (wyChar*)pattern.GetString(),
		       PCRE_COMPLIE_OPTIONS, &res , wyFalse, wyTrue);

	if(matchret != MATCHED)
	{
		m_parseerr = wyTrue;
		return;
	}

	m_prevstr.Erase(0, 1);
	query->SetAs(res);
    return;				
}

void 
SQLFormatter::SetSelectPattern(wyString *selpat, wyInt32 querytype)
{
	//select pattern will set based on the query type
	// if querytype is main query ist case is used to match.
	//1f querytype is sub query  2nd case is used to match.
	//1f querytype is conditional 3rd case is used to match.

	if(querytype == MAIN_QUERY)
		selpat->SetAs("(^EXPLAIN\\s+SELECT|^SELECT)");
	else if(querytype == SUB_QUERY)
	    selpat->SetAs("^\\(\\s*SELECT");

	else
		 selpat->SetAs("SELECT");	

	selpat->AddSprintf("%s[`\\(\\s\\*]", SELECT_OPTS);
}

/*before looking for aliases once verify  all patterns that can use after table name.
if no pattern is matched then take the entire next word as alias.*/
void
SQLFormatter::MatchTableAliasPattern(wyString *query)
{
	wyString  res;
	wyString  pattern;
	wyInt32   matchret, trimcnt;
	wyInt32   spacetrimpos;

	query->LTrim();
	trimcnt = IsSpaceSepChar(query->GetLength());
	//here one space is giving before (alias) or (as alias)
    spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	TraceCommentPos(spacetrimpos, 1 - trimcnt);
	//the following pattern is used for matching aliases
	//this function should call only when table is referred with alias name.  	
	pattern.SetAs(TABLEALIAS); 
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);

	if(matchret == MATCHED)
	{
		m_queryFormate.AddSprintf("%s%s",m_horspace, m_prevstr.GetString());
		query->SetAs(res);
		return;
	}

	if(query->GetLength() != 0)
		m_parseerr = wyTrue;
	
	return;
}

/* here replacing the  new line , tabs and two consequence 
horizantal space follwed by white space with one horizantal space.
*/
void
SQLFormatter::TrimSpaces(wyString *query)
{
	wyString res, pattern, temp;
	wyInt32  matchret, rtrimcnt;
	wyInt32  findpos;
	wyInt32  spacetrimpos;

	temp.SetAs(*query);
	query->SetAs("");

	while(1)
	{
		if(temp.GetLength() == 0)
			return;

		/*replacing space types

		1) new line followed by 0 or more spaces(white spaces)
		2) tab followed by 0 or more spaces(white spaces)
		3) one horizantal tab followed by 1 or more spaces(white spaces)
		*/
		pattern.SetAs(WHITESPACE);	

		matchret = MatchStringAndGetResult(temp.GetString(),(wyChar*)pattern.GetString(), 
			PCRE_UTF8|PCRE_CASELESS, &res, wyFalse, wyTrue);
		
		if(matchret == NOTMATCHED ||res.GetLength() == 0)
		{
			query->Add(temp.GetString());
			return ;
		}
        //IsPatternFindAsVarQualifier
		if(IsEvenBackticks(GetSubStr(&temp, 0, temp.GetLength() - res.GetLength())) == wyFalse)
		{
			if((findpos = res.Find("`", 0)) == -1)
			{
				m_parseerr = wyTrue;
				return ;
			}

			res.Erase(0, findpos + 1);
			query->AddSprintf("%s", GetSubStr(&temp, 0, temp.GetLength() - res.GetLength()));
			temp.SetAs(res);			
			continue;
		}

		//if 'space before comma' sequence is found in a query we are replacing this sequence
		//with single comma
		if(m_prevstr.GetCharAt(m_prevstr.GetLength() - 1) == ',')
		{
			query->AddSprintf("%s,",GetSubStr(&m_prevstr, 0, m_prevstr.GetLength() -2));			
			//here we are trimming only m_matchpatlen - 1 spaces
			//first argument of fun gives at which pos spaces are trimming.
			spacetrimpos = m_querylen - (res.GetLength() + 2);
			TraceCommentPos(spacetrimpos, 1);
		}
		else
		{
			//here new line character is considering as a double characters
			//suppose if we trim one new line character trim count becomes two.			
			m_prevstr.RTrim(&rtrimcnt);
			query->AddSprintf("%s ", m_prevstr.GetString()); 
			//here we are trimming only m_matchpatlen - 1 spaces
			//first argument of fun gives at which pos spaces are trimming.
			spacetrimpos = m_querylen - (res.GetLength() + rtrimcnt);
			TraceCommentPos(spacetrimpos, rtrimcnt - 1);
		}	

		temp.SetAs(res);
	}	
}

//this fun will trace lead spaces of comments
//if one space is trimmmed in query  ,lead space becomes   -1
//if one space is additionally added ,lead spaces becomes  +1
void
SQLFormatter::TraceCommentPos(wyInt32 spacetrimpos, wyInt32 spaces)
{
	wyInt32 cmntcnt = 0;	

	if(m_iscomment == wyFalse || spaces == 0)
		return;

	//use of m_trackcmntno
	//it tells  before which comment current space pos is lies .
	cmntcnt = m_trackcmntno;

	for(;cmntcnt < m_cmntcnt; cmntcnt++)
	{
		if(spacetrimpos + 1 <= m_pcmnts[cmntcnt]->m_startpos)
		{
			m_pcmnts[cmntcnt]->m_leadspaces += spaces;
			m_trackcmntno = cmntcnt;
			break;
		}
	}
}

/*if any space is trimmed in query we have to adjust all the comments pos
which came after trimmed space pos, to that we are substracting the all the 
comments(before which comments trimmed spaces came)startpos with
trimmed spaces*/
void
SQLFormatter::AdjustCommentPosOnTrim()
{	
	wyInt32  cmntcnt = 0, count;

	if(m_iscomment == wyFalse)
		return;

	for(;cmntcnt < m_cmntcnt; cmntcnt++)
	{
		for(count = 0; count <= cmntcnt; count++)
			m_pcmnts[cmntcnt]->m_startpos -= m_pcmnts[count]->m_leadspaces;			
	}

	//set all comment lead spaces to zero
	for(count = 0; count < m_cmntcnt; count++)
		m_pcmnts[count]->m_leadspaces = 0;	

}

//find the query type and formate it
wyBool
SQLFormatter::DoDMSSqlFormate(wyString *query, wyBool ishtmlform)
{
	wyInt32   parseres = -1, trimcnt;
	wyString  uncommstr;	

	query->LTrim();
	query->RTrim();

	if(query->GetLength() == 0)
		return wyTrue;
	//removing the comments from query
	GetQueryWtOutComments(query, &uncommstr);  

	if(ishtmlform == wyTrue && m_iscomment == wyTrue)
		return wyFalse;

	if(m_parseerr == wyTrue)
		return wyFalse;	
	
	query->SetAs(uncommstr);
	m_querylen = query->GetLength();	
	TrimSpaces(query);
	AdjustCommentPosOnTrim();
	m_querylen = query->GetLength();
	m_trackcmntno = 0;
	m_spacerefquery.SetAs(*query);

	//if query start with comment 
	query->LTrim(&trimcnt);
	TraceCommentPos(0, -trimcnt);

	if(query->FindI("SELECT") == 0)
		parseres = FormateSelectStmt(query);	
	else if(query->FindI("EXPLAIN") == 0)
		parseres = FormateSelectStmt(query, wyTrue);
	else if(query->FindI("UPDATE") == 0)
		parseres = FormateUpdateStmt(query);
	else if(query->FindI("DELETE") == 0)
		parseres = FormateDeleteStmt(query);
	else if(query->FindI("INSERT") == 0)
		parseres = FormateInsStmt(query);
	
	//if error occur 
	if(m_parseerr == wyFalse && parseres != -1)
	{
		RestoreCommentsToQuery();	
		FreeCommentBuffer();
		//while restoring comments to the query there is chance of comments restoring failure
		return(m_parseerr == wyFalse)?wyTrue:wyFalse;
	}
	
	FreeCommentBuffer();
	return wyFalse;
}

wyInt32
SQLFormatter::FormateInsStmt(wyString *query)
{
	wyString	pattern, res;
	wyInt32		matchret, leadspaces;	
	wyInt32		trimcnt;

	m_stmttype = INSERT_STMT;

	pattern.SetAs("^INSERT(\\s+LOW_PRIORITY|\\s+DELAYED|\\s+HIGH_PRIORITY)?(\\s+IGNORE)?(\\s+INTO)?[`\\s]"); 
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED)
		return NOTMATCHED;

	m_queryFormate.SetAs(m_prevstr);
	//stripping the last character
	m_queryFormate.Strip(1);

	res.LTrim(&trimcnt);
	InsertSpaces(SPACEAFTERKW);
	TraceCommentPos(m_querylen - res.GetLength(), SPACEAFTERKW - trimcnt);
	
	//searching for table name
	MatchTablePattern(&res);

	if(m_parseerr == wyTrue)
		return NOTMATCHED;
	
	leadspaces = m_queryFormate.GetLength();
	m_queryFormate.AddSprintf("%s", m_prevstr.GetString());

	query->Sprintf("%s", res.GetString());
	FormateInsertQuery(query,leadspaces);

	return MATCHED;
}

wyInt32
SQLFormatter::FormateInsertQuery(wyString *query, wyInt32 leadspaces)
{
	wyInt32 space;
	
	query->LTrim(&space);
	
	if(IsEmptyQuery(query, space) == wyTrue)
		return MATCHED;

	TraceCommentPos(m_querylen - query->GetLength(), -space);

	//if column list is present
	if(query->GetCharAt(0) == '(')
	{
		if(FormateInsColList(query, leadspaces) == NOTMATCHED)
			return NOTMATCHED;
	}

	//checking whether On Duplicate Key Update is present or not
	IsUpdateOnDuplicate(query);
	return MATCHED;
}

void
SQLFormatter::IsUpdateOnDuplicate(wyString *query)
{
	wyString pattern, res, colvalues, tempstr;
	wyInt32  matchret, collen;
	wyInt32  querylen;

	pattern.SetAs("ON\\s*DUPLICATE\\s*KEY\\s*UPDATE[`\\s]");

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

	//if On Duplicate Key Update is present then we are dividing the query into two parts.
	//first part will check for Values,SEt or Select.
    //second part means after On duplicate key update will take as a conditional expression
	if(matchret == MATCHED)
	{
		collen = query->GetLength() -(res.GetLength() + m_matchpatlen - 1);
		//first part till On duplicate key Update
		colvalues.SetAs(GetSubStr(query, 0, collen ));
		tempstr.SetAs(GetSubStr(query, m_matchstartpos, m_matchpatlen - 1));

		//Only first part we are sending to this function so we need to set the querylen also.
		querylen = m_querylen;
		m_querylen = m_querylen - (res.GetLength() + m_matchpatlen - 1);
		FormateInsColValues(&colvalues);
		m_querylen = querylen;

		InsertSpaces(m_leadspaces, wyTrue);
		TraceCommentPos(m_querylen - res.GetLength()- tempstr.GetLength(), m_leadspaces + NEWLINELENGTH);
        
		m_queryFormate.AddSprintf("%s", tempstr.GetString());
		InsertSpaces(SPACEAFTERKW);
		TraceCommentPos(m_querylen - res.GetLength(), SPACEAFTERKW);
		HandleCondExp(&res, res.GetLength(), m_matchpatlen);
	}
	else
		//if on duplicate key is not present then whole query we are sending to this function.
		FormateInsColValues(query);	

	return;
}

wyInt32
SQLFormatter::FormateInsColValues(wyString *query)
{
	wyString	pattern, res;
	wyInt32		matchret;
	wyInt32		noofspaces, querylen, trimcnt;

	query->LTrim(&trimcnt);
	
	pattern.SetAs("^VALUES|^VALUE|^SET|^SELECT[`\\s]");		
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED)
	{
		m_parseerr = wyTrue;
		return NOTMATCHED;
	}

	querylen = m_querylen - query->GetLength();
	noofspaces =  m_leadspaces + NEWLINELENGTH - trimcnt;
	InsertSpaces(m_leadspaces, wyTrue);
	TraceCommentPos(querylen, noofspaces);

	if(query->FindI("SELECT") == 0)
	{
		m_stmttype = SELECT_STMT;
		FormateSelectStmt(query);
		m_stmttype = INSERT_STMT;
	}
	else if(query->FindI("SET") == 0)
	{	
		m_queryFormate.AddSprintf("%s", m_prevstr.GetString());
		//removing the seperator character
		res.Erase(0,1);
		// adding one space after "SET" clause and adjust that space with comment position.
		InsertSpaces(SPACEAFTERKW);
		querylen = m_querylen - res.GetLength();
		TraceCommentPos(querylen, SPACEAFTERKW);

		//if it is SET clause we are handling same as SET clause in UPDATE stmt.
		//In INSERT SET clause and UPDATE SET clause same column pattern we are using
		//for this purpose we are setting stmt type to update stmt
		m_stmttype = UPDATE_STMT;
		FormateSetClause(&res);
		m_stmttype = INSERT_STMT;
	}
	else
	{
		//removing the seperator character
		res.Erase(0,1);
		FormateValuesClause(&res);
	}
	
	return 1;
}

void
SQLFormatter::FormateValuesClause(wyString* query)
{
	wyInt32		spacetrimpos, querylen;
	wyInt32		leadspaces, len, trimcnt;
	wyString	condexp;

	m_queryFormate.AddSprintf("%s", m_prevstr.GetString());
	
	// adding one space after "SET" clause and adjust that space with comment position.
	InsertSpaces(SPACEAFTERKW);
	spacetrimpos = m_querylen - query->GetLength();
	TraceCommentPos(spacetrimpos, SPACEAFTERKW);
	leadspaces = m_queryFormate.GetLength() - NewLineLastOccurrence(m_queryFormate.GetString()) - 1;

	while(1)
	{
		query->LTrim(&trimcnt);
		if(IsEmptyQuery(query, trimcnt) == wyTrue)
			return ;

		spacetrimpos = m_querylen - query->GetLength(); 
		TraceCommentPos(spacetrimpos, -trimcnt);
		query->Erase(0, 1);
		
		if((len = GetStrLenWithInParanthesis(query->GetString(), wyTrue)) == -1)
		{
			m_parseerr = wyTrue;
			return ;
		}

		condexp.SetAs(GetSubStr(query, 0, len));	
		m_queryFormate.Add("(");	

		//if bulk insert stmt, then we can insert multiple rows with one insert stmt
		//so we are sending only one row at a time, then checking for comma.
		//if comma found next row we are sending to this function
		querylen = m_querylen;
		m_querylen = m_querylen - (query->GetLength() - len);
		HandleValuesClauseForInsert(&condexp, leadspaces + 1 );
		m_querylen = querylen;
		
		m_queryFormate.Add(")");
		
		query->SetAs(GetSubStr(query, len + 1, query->GetLength() - len));
		query->LTrim(&trimcnt);
		spacetrimpos = m_querylen - query->GetLength();
		TraceCommentPos(spacetrimpos, -trimcnt); 
		
		if(query->GetCharAt(0) == ',')
		{
			query->Erase(0, 1);
			
			if(m_opts->m_stacked == wyTrue)
			{
				if(m_opts->m_linebreak == AFTERCOMMA)
				{
					m_queryFormate.Add(",");
					/*if comment comes after comma and before table we have to move
					this comment to next line because if it come after new line 
					then only it looks good, to come like that we are moving current query pos to one pos back*/
					InsertSpaces(leadspaces, wyTrue);
					spacetrimpos = m_querylen - (query->GetLength() + 1);
					TraceCommentPos(spacetrimpos, leadspaces + NEWLINELENGTH);
				}	
				else
				{
					InsertSpaces(leadspaces, wyTrue);
					spacetrimpos = m_querylen - (query->GetLength() + 1);
					TraceCommentPos(spacetrimpos, leadspaces + NEWLINELENGTH);
					m_queryFormate.Add(", ");
					TraceCommentPos(m_querylen - query->GetLength(), 1);

				}
		
			}
			else
			{
					m_queryFormate.Add(", ");
					TraceCommentPos(m_querylen - query->GetLength(), 1);
			}
		}
	}

	return;
}

wyInt32 
SQLFormatter::HandleValuesClauseForInsert(wyString* query, wyInt32 leadspaces)
{	
	wyString	pattern;
	wyString	res;
	wyString	condexp, tempstr;
	wyInt32		len, matchret, spacetrimpos;	
	wyInt32		trimcnt;
	wyString	searchchar;
	wyInt32		pos = 0;
	
	//Here we are searching for comma and open parenthesis(for sub query)
	pattern.SetAs(",|\\(");

	while(1)
	{
		query->LTrim(&trimcnt);
		spacetrimpos = m_querylen - query->GetLength();
		TraceCommentPos(spacetrimpos, -trimcnt);

		matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
			PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
		if(matchret == NOTMATCHED)
		{
			condexp.AddSprintf("%s", query->GetString());
			HandleCondExp(&condexp, 0, leadspaces);
			query->SetAs(res);
			return SUCCESS;
		}

		tempstr.SetAs(GetSubStr(query, m_matchstartpos, m_matchpatlen));

		if(IsPatternFindAsVarQualifier(&m_prevstr, &searchchar) == wyTrue)
		{
			if((pos = res.Find(searchchar.GetString(), 0)) == -1 )
			{
				m_parseerr = wyTrue;
				return wyFalse;
			}

			condexp.AddSprintf("%s", GetSubStr(query, 0, m_matchendpos + pos));
			res.Erase(0, pos + 1);
			query->SetAs(res);
			continue;
		}

		if(tempstr.FindIWithReverse("(", 0, wyTrue) == 0)
		{
			res.Erase(0, 1);
			len = GetStrLenWithInParanthesis(res.GetString(), wyTrue);

			if(len == -1)
			{			
				m_parseerr = wyTrue;
				return wyTrue;
			}
			
			condexp.AddSprintf("%s", GetSubStr(query, 0, m_matchstartpos + m_matchpatlen + len + 1));
			query->Sprintf("%s", GetSubStr(&res, len + 1, res.GetLength() - len - 1));	
			continue;
		}
		
		condexp.AddSprintf("%s", GetSubStr(query, 0, m_matchendpos - 1));
		HandleCondExp(&condexp, res.GetLength(), leadspaces);
		
		if(tempstr.GetCharAt(0) == ',')
		{			
			res.Erase(0, 1);
			if(m_opts->m_stacked == wyTrue)
			{
				if(m_opts->m_linebreak == AFTERCOMMA)
				{
					m_queryFormate.Add(",");
					/*if comment comes after comma and before table we have to move
					this comment to next line because if it come after new line 
					then only it looks good, to come like that we are moving current query pos to one pos back*/
					InsertSpaces(leadspaces, wyTrue);
					TraceCommentPos(m_querylen - (res.GetLength() + 1), leadspaces + NEWLINELENGTH);
				}	
				else
				{
					InsertSpaces(leadspaces, wyTrue);
					TraceCommentPos(m_querylen - (res.GetLength() + 1), leadspaces + NEWLINELENGTH);
					m_queryFormate.Add(", ");
					TraceCommentPos(m_querylen - res.GetLength(), 1);
				}
			}
			else
			{
				m_queryFormate.Add(", ");
				TraceCommentPos(m_querylen - res.GetLength(), 1);
			}
		}
		
		query->SetAs(res);
		condexp.SetAs("");
	}
}

wyInt32
SQLFormatter::FormateInsColList(wyString* query, wyInt32 leadspaces)
{
	wyInt32		trimcnt, len;   
	wyBool		colpar;
	wyString	collist, temp;
	wyInt32		querylen;
	wyInt32		spacetrimpos;

	InsertSpaces(leadspaces, wyTrue);	
	TraceCommentPos(m_querylen - query->GetLength(), leadspaces + NEWLINELENGTH);
	query->Erase(0, 1);

	if((len = GetStrLenWithInParanthesis(query->GetString(), wyTrue)) == -1)
		return NOTMATCHED;	

	collist.SetAs(GetSubStr(query, 0, len));	
	temp.SetAs(GetSubStr(query, len + 1, query->GetLength() - len));
	query->SetAs(temp);
	querylen = m_querylen - (query->GetLength() + 1);
	m_queryFormate.Add("(");

	//leadspaces is increasing by 1, second column we are showing below the first column level 
	leadspaces = leadspaces + 1;

	while(1)
	{		
		//adjusting the space before column name
		collist.LTrim(&trimcnt);
		spacetrimpos = querylen - collist.GetLength();
		TraceCommentPos(spacetrimpos, -trimcnt);
		colpar = MatchColumnPattern(&collist);
		
		//adjusting the space after column name(ie, before comma)
		collist.LTrim(&trimcnt);
		spacetrimpos = querylen - collist.GetLength();
		TraceCommentPos(spacetrimpos, -trimcnt);

		if(colpar == wyTrue && collist.GetCharAt(0) == ',')
		{
			//erasing the comma character
			collist.Erase(0, 1);
			
			if(m_opts->m_stacked == wyTrue)
			{
				if(m_opts->m_linebreak == AFTERCOMMA)
				{
					m_queryFormate.Add(",");
					/*if comment comes after comma and before table we have to move
					this comment to next line because if it come after new line 
					then only it looks good, to come like that we are moving current query pos to one pos back*/
					InsertSpaces(leadspaces ,wyTrue);
					spacetrimpos = querylen - (collist.GetLength() + 1);
					TraceCommentPos(spacetrimpos, leadspaces + NEWLINELENGTH);
				}	
				else
				{
					//adjusting the spaces before comma
					InsertSpaces(leadspaces , wyTrue);
					spacetrimpos = querylen - collist.GetLength() - 1;
					TraceCommentPos(spacetrimpos, leadspaces + NEWLINELENGTH);
					
					//after comma we are adding one space and adjusting that space
					m_queryFormate.Add(", ");
					spacetrimpos = querylen - collist.GetLength();
					TraceCommentPos(spacetrimpos, 1);
				}
			}
			else
			{
				//after comma we are adding one space and adjusting that space
				m_queryFormate.Add(", ");
				spacetrimpos = querylen - collist.GetLength();
				TraceCommentPos(spacetrimpos, 1);
			}
			continue;
		}
		
		//if column pattern is not matched or 
		//after column any other characters except comma comes means wrong query.
		if(colpar == wyFalse || collist.GetLength() != 0)
		{
			m_parseerr =wyTrue;
			return NOTMATCHED;
		}

		break;
	}

	m_queryFormate.Add(")");
	return MATCHED;
}

/*
here columns (aliases) are aligned(formatting) by the following way.
1) parsing all the columns and  aliases, after that saving these values in a structure,
2)spaces b.w columns and aliases are given by considering the maximum column lengths.
NOTE: here subquery and function column lengths	are ignored for calculating
max col lenths.
*/
//parse all different formates  of select_expressions
wyInt32
SQLFormatter::ParseColumnLevel(wyString *query)
{
	wyChar      character;
	wyString    pattern, res, temp;	
	wyInt32     ret, trimcnt, pos;
	ColumnList  **pcollist = NULL;	
	wyInt32     colcnt = 0,bfrefrom;
	wyBool     isfrommatch = wyFalse;
	wyString   collist;
	wyInt32    spacetrimpos, nospaces;
			
	query->LTrim();

	trimcnt = IsSpaceSepChar(query->GetLength());

	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	//here no of spaces adding before column are m_leadspaces + m_opts->m_spaceindentation and 
	// one new line
	if(m_opts->m_stacked == wyTrue)
	{
		InsertSpaces(m_leadspaces + m_opts->m_spaceindentation, wyTrue); 
		nospaces = m_leadspaces + m_opts->m_spaceindentation + NEWLINELENGTH - trimcnt;
		TraceCommentPos(spacetrimpos, nospaces);
	}
	else	
		TraceCommentPos(spacetrimpos, -trimcnt);

	temp.SetAs(*query);
	
	while(1)
	{
		query->LTrim(&trimcnt);

		if(query->GetLength() == 0)
		{
			ret = MATCHED;
			goto OnError;
		}

		//for stack mode 
		if(m_opts->m_stacked == wyTrue)
		{
			colcnt++;
			m_collist = (ColumnList*)malloc(sizeof(ColumnList));
			pcollist = (ColumnList**)realloc(pcollist, sizeof(ColumnList*) *(colcnt));	
			pcollist[colcnt - 1] = m_collist;
			m_collist->m_alias = NULL;
			m_collist->m_col = NULL;
			m_collist->m_askw = NULL;
		}
		else
		{
			//in non stack mode one space is adding after each comma
			m_queryFormate.AddSprintf("%s", m_horspace);
			spacetrimpos = m_querylen - query->GetLength() - trimcnt;
			TraceCommentPos(spacetrimpos, 1 - trimcnt);
		}

		character = query->GetCharAt(0);

		//if first character is '(' col type should be either subquery type or 
		//columns are enclosed inside brackets
		if(character == '(')
		{	
			if(m_opts->m_stacked == wyTrue)
				m_collist->m_collength = GetStrLenWithInParanthesis(query->GetString() + 1, wyTrue) + 2;

			if(HandleColumnSubQuery(query ) == wyFalse)
				ColumnEnclosedByParanthesis(query);			
		}
		else if(IsFunAsColumn(query) == wyFalse)		
		{
			if(MatchColumnPattern(query) == wyFalse)
			{
				ret = NOTMATCHED;
				goto OnError;
			}
		}

		if(m_parseerr == wyTrue)
		{
			ret = NOTMATCHED;
			goto OnError;			
		}

		if((ret = MatchColumnAliases(query, isfrommatch)) != CONTINUE_PARSE)	
		{
			if(ret == NOTMATCHED)
				goto OnError;

			break;
		}

	}

	//collist string used for space reference in the column list
	//through these string only we are finding the spaces before and after comma in a query
	collist.Sprintf("%s", GetSubStr(&temp, 0, temp.GetLength() - query->GetLength()));	
	FormateColumnList(pcollist, &collist, colcnt, query->GetLength());	

	//if form clause used
	if(isfrommatch == wyTrue)
	{
		// pos is 'from' pos in query
		pos = temp.GetLength() - query->GetLength() - FROMKWLENGTH - 1;
		//character before from clause 
		bfrefrom  = temp.GetCharAt(pos) == ' ' ? 1: 0;		
		query->LTrim(&trimcnt);
		FormateFrom(query->GetLength() + trimcnt, bfrefrom,  trimcnt, GetSubStr(&temp, pos + 1, 4));		
	}
	
	ret = MATCHED;
OnError:
	FreeColumnList(pcollist, colcnt);
	return ret;	
}

//adding from key word with case (useful with Unchanged keyword case option) to the formatting query
void
SQLFormatter::FormateFrom(wyInt32 querylen,  wyInt32 bfrefrom, wyInt32 afterfrom, wyChar *from)
{
	wyInt32  spacetrimpos, nospaces;

	//lead spaces before from clause
	spacetrimpos = m_querylen - querylen - FROMKWLENGTH - bfrefrom;
	nospaces = m_leadspaces + NEWLINELENGTH - bfrefrom;
	TraceCommentPos(spacetrimpos, nospaces);	
	InsertSpaces(m_leadspaces, wyTrue);

	if(!from)
	{
		m_parseerr = wyTrue;
		return;
	}

	m_queryFormate.AddSprintf("%s", from);	
	InsertSpaces(SPACEAFTERKW);
	//here first argument is len after from clause including space(if exist)
	spacetrimpos = m_querylen - querylen;
	nospaces = SPACEAFTERKW - afterfrom;
	TraceCommentPos(spacetrimpos, nospaces);
	//	TraceCommentPos(m_querylen - querylen, SPACEAFTERKW - afterfrom);
}

/* 
column formation is done based on max column length. 
stpes to column formation.
1)adding  column name and giving the spaces obtained from the result (maxcollen - collen).
2)Adding AS keyword (if it used to give alias)  + 1 space.
3) if "AS" key word is not there add 4 spaces (AS + one space) for AS keyword alignment
4)finally  adding column aliases. 
*/
void
SQLFormatter::FormateColumnList(ColumnList **pcollist, wyString *selcolstr, wyInt32 colcnt, wyInt32 reslen)
{
	wyInt32    cnt = 0;
	wyInt32    nospaces;
	ColumnList *collist;
	wyString   str;
	wyInt32    templen = 0;
	wyInt32    trimcnt =0, bfrecomma = 0;
	wyInt32    aftercomma = 0, temptrimcnt =0;
	wyInt32    curquerypos =0, pos =0,collen =0;
	wyInt32    spacetrimpos;
	
	m_maxcollen += 1;
	//here max_collen is incrementing to one for including space 
	pos = m_querylen - reslen;
	
	for( ; (cnt < colcnt) && pcollist[cnt] && pcollist[cnt]->m_col  ;cnt++)
	{
		str.SetAs(" ");
		collist = pcollist[cnt];		
		
		if(cnt != 0)
		{
			templen = 0;

			if(m_iscomment == wyTrue)
			{
				curquerypos = pos - selcolstr->GetLength();
				selcolstr->Erase(0, 1);
				selcolstr->LTrim(&aftercomma);
			}

			LineBreaksWithComma(curquerypos, bfrecomma, aftercomma);		
		}
		else
		{			
			//if BEFORE COMMA option is selected,  no of spaces after comma is 
			//lead spaces + two spaces 
			if(colcnt > 1 && m_opts->m_linebreak == BEFORECOMMA)
				templen = 2;
		}
        
		collen = collist->m_collength;
		m_queryFormate.AddSprintf("%s",  collist->m_col);

		if(m_iscomment == wyTrue)
		{
			selcolstr->Erase(0, collen);
			selcolstr->LTrim(&trimcnt);
		}

		if(!collist->m_alias)
		{
			bfrecomma = trimcnt;
			continue;
		}		

		if(m_opts->m_colalign == wyTrue)
		{
			if(m_maxcollen >= (collen + 1))
                nospaces = m_maxcollen - collen + templen; 
			else
				nospaces = 1; 

			InsertSpaces(nospaces);
			temptrimcnt = nospaces - trimcnt;

			//if askeyword used to alias the column
			if(collist->m_askw == NULL)
			{
				//adding three spaces to the formatted query instead of 'AS' keyword.
				//for aligning with alias
				str.Sprintf("%s%s%s", m_horspace, m_horspace, m_horspace);
				temptrimcnt += 3;
			}
		}
		else
		{
			if(collist->m_askw != NULL)
				m_queryFormate.AddSprintf("%s", m_horspace);

			//if space is there no need to adjust comment pos if not there then 
			// only we need to adjust comment positions
			temptrimcnt = 1 - trimcnt;
		}
		
		//here we are aligning 	columns and its aliases only if m_colalign is set to true		 
		if(collist->m_askw != NULL)
		{
			spacetrimpos = pos - selcolstr->GetLength() - trimcnt;
			TraceCommentPos(spacetrimpos, temptrimcnt);
			m_queryFormate.AddSprintf("%s%s%s", collist->m_askw, m_horspace, collist->m_alias);
			selcolstr->Erase(0, 2);
			selcolstr->LTrim(&trimcnt);
			temptrimcnt = 1 - trimcnt;
		}
		else
			m_queryFormate.AddSprintf("%s%s", str.GetString(), collist->m_alias);

		if(m_iscomment == wyFalse)
			continue;

		spacetrimpos = pos - selcolstr->GetLength() - trimcnt;
		TraceCommentPos(spacetrimpos, temptrimcnt);
		selcolstr->Erase(0, strlen(collist->m_alias));
		selcolstr->LTrim(&bfrecomma);
	}	
}

/*fun used for adding line breaks after table and column*/
//curquerypos : the comma position in query
//befre comma :space is used or not
//after comma :space is used or not
void 
SQLFormatter::LineBreaksWithComma(wyInt32 curquerypos, wyInt32 bfrecomma, wyInt32 aftercomma)
{
	wyInt32  spacetrimpos, nospaces;

	if(m_opts->m_linebreak == AFTERCOMMA)
	{
		spacetrimpos = curquerypos - bfrecomma;
		TraceCommentPos(spacetrimpos, -bfrecomma);
		m_queryFormate.AddSprintf(",%s", m_newline);
		/*if comment comes after comma and before table we have to move
		this comment to next line because if it come after new line 
		then only it looks good, to come like that we are moving current query pos to one pos back*/
		spacetrimpos = curquerypos + 1 - aftercomma;
		nospaces = m_leadspaces + m_opts->m_spaceindentation + NEWLINELENGTH -  aftercomma;
		TraceCommentPos(spacetrimpos, nospaces);
		InsertSpaces(m_leadspaces + m_opts->m_spaceindentation);
	}	
	else
	{
		spacetrimpos = curquerypos - bfrecomma;
		nospaces = m_leadspaces + m_opts->m_spaceindentation + NEWLINELENGTH - bfrecomma;
		TraceCommentPos(spacetrimpos, nospaces);
		InsertSpaces(m_leadspaces + m_opts->m_spaceindentation, wyTrue);
		m_queryFormate.AddSprintf(",%s", m_horspace);
		spacetrimpos = curquerypos + 1;
		nospaces = 1 - aftercomma;
		TraceCommentPos(spacetrimpos, nospaces);
	}
}

//inserting horizantal spaces and new line to the formatted query  
void 
SQLFormatter::InsertSpaces(wyInt32 nospaces, wyBool isnewline)
{
	wyChar *spacebuff = NULL;
	
	if(isnewline == wyTrue)
		m_queryFormate.Add(m_newline);	

	if(nospaces <= 0)
		return;

	if(m_horspace[0] != ' ')
		return InsertHtmlHorizantalSpace(nospaces);	

	spacebuff = (wyChar*)calloc(nospaces + 1, sizeof(wyChar));
	
	if(!spacebuff)
		free(spacebuff);

	memset(spacebuff, ' ', nospaces);
	m_queryFormate.AddSprintf("%s", spacebuff);

	if(spacebuff)
		free(spacebuff);	
}

void 
SQLFormatter::InsertHtmlHorizantalSpace(wyInt32 nospaces)
{
	while (nospaces--)	
		m_queryFormate.Add(m_horspace);
}

//in this fun getting the column aliases and saving aliases in buffer.
//this fun will also give selected column list completion
wyInt32 
SQLFormatter::MatchColumnAliases(wyString *query, wyBool &isfrommatch)
{
	wyString pattern, res, temp;	
	wyInt32  matchret, spacetrimpos;
	wyInt32  spaceafteraskw =0, spacebfreaskw = 0;
	wyInt32  ret;		
	wyString collation;
	wyString askw;

	if((ret = CheckColumnListCompletion(query, isfrommatch)) != NOTMATCHED) 
		return ret;	

	//if collation used  in a query to select the column 
	if(query->CompareNI("COLLATE", 7) == 0)
	{
        FormateCollationSyntax(query);

        if((ret = CheckColumnListCompletion(query, isfrommatch)) != NOTMATCHED) 
			return ret;	
	}	

	pattern.SetAs("^AS\\s*(`[^`]+`|\'[^\']+\'|\"[^\"]+\"|[^,|^\\s]+)|(^`[^`]+`|^\"[^\"]+\"|^'[^']+'|[^,|^\\s]+)");	

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);

	if(matchret == NOTMATCHED)
		return NOTMATCHED;	

	//if alias comes along with 'AS' keyword
	if(m_prevstr.FindI("AS") == 0)
	{
		askw.SetAs(m_prevstr.Substr(0, 2));
		m_prevstr.Erase(0, 2);		
	}

	//in non stack mode only we are considering these spaces . 	
	 m_prevstr.LTrim(&spaceafteraskw);	

	//if nonstackmode is selected for listed columns
	if(m_opts->m_stacked == wyFalse)
	{
		spacetrimpos = m_querylen - query->GetLength();
		spacebfreaskw = IsSpaceSepChar(query->GetLength());
		TraceCommentPos(spacetrimpos - spacebfreaskw, 1 - spacebfreaskw);

		//if as kwy word is there one space is adding before and AFTER AS KEY WORD.
		//like "one space AS onespace aliasname"
		if(askw.GetLength() != 0)
		{
			m_queryFormate.AddSprintf("%s%s%s%s", m_horspace, askw.GetString(), m_horspace, m_prevstr.GetString());
			spacetrimpos = m_querylen - query->GetLength() + 2; //here two is  For as keyword length
			TraceCommentPos(spacetrimpos, 1 - spaceafteraskw);			
		}
		else// "one space aliasname"
			m_queryFormate.AddSprintf("%s%s", m_horspace, m_prevstr.GetString());

		query->SetAs(res);		
		return CheckColumnListCompletion(query, isfrommatch);
	}

	query->SetAs(res);

	if(askw.GetLength() != 0)
	{
		m_collist->m_askw = (wyChar*)malloc(sizeof(wyChar)*2 + 1);	
		strcpy(m_collist->m_askw, askw.GetString());
	}

	m_collist->m_alias = (wyChar*)malloc(m_prevstr.GetLength() + 1);

	if(!m_collist->m_alias)
		return NOTMATCHED;	

	strcpy(m_collist->m_alias, (wyChar*)m_prevstr.GetString());	
	return CheckColumnListCompletion(query, isfrommatch);
}


void
SQLFormatter::FormateCollationSyntax(wyString *query)
{
	wyString collation, collationkw, collationstr;
	wyInt32  trimcnt, spacetrimpos, tempquerylen;	 	

	tempquerylen = query->GetLength();
	collationkw.SetAs(GetSubStr(query, 0, 7));		
	query->Erase(0, 7);
	query->LTrim();

	GetCollation(query, &collation);
	query->Erase(0, collation.GetLength());

	//trace the comment pos b/w column name and collation
	trimcnt = IsSpaceSepChar(tempquerylen);
	spacetrimpos = m_querylen - query->GetLength();
	TraceCommentPos(spacetrimpos - trimcnt, 1 - trimcnt);

	//here column name , collation and collation type all are taking in a single column buffer as a single column
	//like column name collation collation type.
	if(m_opts->m_stacked == wyTrue)
	{		
		m_trackcmntno = 0; 		
		//we are constanly giving one space between column and collation even though
		// query  doesnot have any space between column and collation 
		//so for that  we need to trace the column length.
		collationstr.Sprintf("%s%s%s%s%s", m_collist->m_col, m_horspace, collationkw.GetString(), 
								m_horspace, collation.GetString());

		m_collist->m_col = (wyChar*)realloc(m_collist->m_col,  collationstr.GetLength());
		strcpy(m_collist->m_col , collationstr.GetString());
		// if space is not there in a query b/w column name and collation  actuval length of column 
		//becomes more than present column length in a query
		//hence we are decrementing the column length by one for tracing comment pos.
		m_collist->m_collength +=  collationkw.GetLength() + collation.GetLength() + 
									1 - trimcnt + 2 * strlen(m_horspace);

		//here some times m_collist->m_collength > strlen(m_collist->m_col)
		//because we have trimmed the spaces between charset and string value
		//ex: _utf8 '12213331' becomes  to  _utf8'12213331' we are finding the spaces 
		//in a query using the main query so we need to adjust column length
	}
	else						
		m_queryFormate.AddSprintf("%s%s%s%s", m_horspace, collationkw.GetString(), m_horspace, 
											  collation.GetString());  										
}

/*it will give the selected column list is completed or not. we can find it by checking
string starting character or word (comma or FROM clause). if query start with 'from' caluse
means parsing of  selected columns is completed */
wyInt32
SQLFormatter::CheckColumnListCompletion(wyString *query, wyBool &isfrommatch)
{
	wyString pattern, res;	
	wyInt32  matchret, trimcnt = 0;
	wyInt32  spacetrimpos;

	query->LTrim(&trimcnt);

	if(IsEmptyQuery(query, trimcnt) == wyTrue)
		return MATCHED;

	if(query->GetCharAt(0) == ',')
	{
		if(m_opts->m_stacked == wyFalse)
		{
			spacetrimpos = m_querylen - query->GetLength() - trimcnt;			
			TraceCommentPos(spacetrimpos, -trimcnt);
			m_queryFormate.Add(",");
		}

		query->Erase(0, 1);
		return CONTINUE_PARSE;
	}	

	pattern.SetAs("^FROM[`\\(\\s]");
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

	if(matchret == MATCHED)
	{
		isfrommatch = wyTrue;
		query->Sprintf("%s", res.GetString());
		return MATCHED;
	}

	return NOTMATCHED;
}

void
SQLFormatter::GetCollation(wyString *query, wyString *collation)
{
	wyInt32   querylen, count = 0;
	wyChar    character;
	

	querylen = query->GetLength();

	if(!querylen)
		return;

	for(; count < querylen; count++)
	{
		character = query->GetCharAt(count);

		if(isspace(query->GetCharAt(count)) != 0 || character == '\'' || character == '"' || 
			character == ',')		
			break;			
	}

	collation->SetAs(query->Substr(0, count));
}


/*query :SELECT _UTF8'string' .here first fetching the charset name from the query 
by matching the charset  using the following pattern and finding the charset is present in the 
charset list or not if not treat the matched string as a column name*/
wyBool
SQLFormatter::IsCharsetUsedToSelString(wyString *query)
{
	wyString  pattern, res;
	wyInt32   matchret;	
	wyChar    **searchres;
	wyInt32   charsetcount;
	wyChar    *charset;
	wyInt32   charsize, ltrimcnt;
	wyString  colname;
	wyChar    charat0pos;
	wyInt32   findpos;

	pattern.SetAs("^_[^\\s^\"^']+|^[bx](\"[^\"]+\"|'[^']+')");

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);

	if(matchret == NOTMATCHED)		
		return wyFalse;

	if(m_prevstr.CompareNI("X", 1)  == 0 || m_prevstr.CompareNI("B", 1) == 0)	
	{
		GetColumn(&m_prevstr, wyTrue);
		query->SetAs(res);
		return wyTrue;
	}

    m_prevstr.Erase(0, 1);

	charsize = sizeof(wyChar*);
	charsetcount = sizeof(charsetlist)/sizeof(charsetlist[0]);

	//charset array list is sorting only once.	
	if(m_issort == wyTrue)
	{				
		qsort((void*)charsetlist, charsetcount, charsize, CharsetAndCollationCmp);	
		m_issort = wyFalse;
	}

	charset = (wyChar*)malloc(charsize * (m_prevstr.GetLength() + 1));

	if(!charset)
		return wyFalse;

	strcpy(charset, m_prevstr.GetString());
	colname.Sprintf("_%s", m_prevstr.GetString());

	searchres = (wyChar**)bsearch(&charset, charsetlist, charsetcount, charsize, CharsetAndCollationCmp);

	//there is chance of having  similarity in column names and charset names 
	if(!searchres)
	{
		free(charset);
		return wyFalse;	
	}

	res.LTrim(&ltrimcnt);

	if(res.CompareNI("X", 1)  == 0 || res.CompareNI("B", 1) == 0)
	{
		colname.AddSprintf("%s%c", m_horspace, res.GetCharAt(0)); 	
		res.Erase(0, 1);
		ltrimcnt = 0;
	}

	charat0pos = res.GetCharAt(0);
	
	if(charat0pos  != '"'  && charat0pos != '\'')
	{
		free(charset);
		m_parseerr = wyTrue;
		return wyTrue;
	}

	//find the next Doblue quote or Single quote occurence
	if((findpos = res.FindChar(charat0pos, 1)) == -1)
	{
		free(charset);
		m_parseerr = wyTrue;
		return wyTrue;
	}

	TraceCommentPos(m_querylen - res.GetLength() - ltrimcnt, -ltrimcnt);	
	colname.Add(GetSubStr(&res, 0, findpos + 1));
	res.Erase(0, findpos + 1);
	GetColumn(&colname, wyTrue);

	//in stack mode we are finding the spaces before and after "column" or "as" or "alias" with
	//the column length reference so here if we trim column length buffer we should 
	//maintain length to the column buffer
	if(m_opts->m_stacked == wyTrue)
	{
		m_collist->m_collength += ltrimcnt; 
		//here in stack mode always set trace comment position to zero. because of that 
		//trace will start from the begin.
		m_trackcmntno = 0;
	}

	query->SetAs(res);
	free(charset);

	return wyTrue;
}

wyInt32
SQLFormatter::CharsetAndCollationCmp(const void *arg1, const void *arg2)
{
	 /* Compare all of both strings: */
   return _stricmp( * ( char** ) arg1, * ( char** ) arg2 );
}


wyBool
SQLFormatter::MatchColumnPattern(wyString *query)
{
	wyString pattern, res, temp;	
	wyInt32  matchret, trimcnt, ltrimcnt;

    //bug with follwoing type of query syntax(7.5 beta1).
	//<_charset> <x>'string value' collate <collation>
	if(m_stmttype == SELECT_STMT && IsCharsetUsedToSelString(query) == wyTrue)
		 return wyTrue;	
   
    //gets the column pattern with backticks for select stmt 
	GetColumnPatternWithBacticks(&pattern);

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);

	if(matchret == MATCHED)
	{
		if(m_stmttype == UPDATE_STMT)
		{
			m_prevstr.Strip(1);//stripping the "=" sign
			m_prevstr.RTrim(&trimcnt); 
			TraceCommentPos(m_querylen - res.GetLength() - 1, 1 - trimcnt);
			res.LTrim(&ltrimcnt);
			TraceCommentPos(m_querylen - res.GetLength(), 1 - ltrimcnt); 
			m_prevstr.Add(" = ");//adding = 
		}

		GetColumn(&m_prevstr, wyFalse);
		query->SetAs(res);
		return wyTrue;
	}

	temp.Sprintf("`%s", query->GetString());
    
	//gets the column pattern without backticks for select stmt
	GetColumnPatternWithOutBacticks(&pattern);

	matchret = MatchStringAndGetResult(temp.GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);
	
	if(matchret == MATCHED)
	{
		m_prevstr.Erase(0, 1);//removing the single quote character 
		if(m_stmttype == UPDATE_STMT)
		{
			m_prevstr.Strip(1);//stripping the "=" sign
			m_prevstr.RTrim(&trimcnt);
			TraceCommentPos(m_querylen - res.GetLength() - 1, 1 - trimcnt);
			res.LTrim(&ltrimcnt);
			TraceCommentPos(m_querylen - res.GetLength(), 1 - ltrimcnt); 
			m_prevstr.Add(" = ");
		}

		GetColumn(&m_prevstr, wyFalse);
		query->SetAs(res);
		return wyTrue;		
	}

	return wyFalse;
}

void
SQLFormatter::GetColumn(wyString *column, wyBool isfun)
{
	//If it is update stmt or insert stmt then we are simply adding Column name to the formatted query
	if(m_opts->m_stacked == wyFalse || m_stmttype != SELECT_STMT)
	{
		m_queryFormate.Add(column->GetString());		
		return;
	}

	//if func type column
	if(isfun == wyFalse && m_maxcollen < column->GetLength())
		m_maxcollen = column->GetLength();

	m_collist->m_col = (wyChar*)malloc(column->GetLength() + 1);

	if(m_collist->m_col)
		strcpy(m_collist->m_col, (wyChar*)column->GetString());

	m_collist->m_collength =column->GetLength();		

	return;
}

/* funs name parsing .these function using 
for matching diff formates of functionnames*/
wyInt32 
SQLFormatter::IsFunAsColumn(wyString *query)
{
	wyString   pattern, res;
	wyString   temp;
	wyInt32    matchret, len;
	wyString   funname;

	pattern.SetAs(FUNASCOL_WTBKQUOTES);	
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);	

	if(matchret == MATCHED)
	{
		funname.SetAs(m_prevstr);		
		len = GetStrLenWithInParanthesis(res.GetString(), wyTrue);

		//if corresponding paranthesis is not found.
		if(len == -1)
		{
			m_parseerr = wyTrue;
			return wyTrue;
		}

		funname.AddSprintf("%s%s", m_newline, GetSubStr(&res, 0, len+2));
		query->Sprintf("%s", GetSubStr(&res, len+1, res.GetLength() - (len+1)));	

		GetColumn(&funname, wyTrue);		
		return wyTrue;
	}

	temp.Sprintf("`%s", query->GetString());

	pattern.SetAs(FUNASCOL_WTOUTBKQUOTES);
	matchret = MatchStringAndGetResult(temp.GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyFalse, wyTrue);

	
	if(matchret == MATCHED)
	{
		//to remove back quotes
		m_prevstr.Erase(0, 1);
		funname.SetAs(m_prevstr);		
		len = GetStrLenWithInParanthesis(res.GetString(), wyTrue);

		//if corresponding paranthesis is not found.
		if(len == -1)
		{
			m_parseerr = wyTrue;
			return wyTrue;
		}

		funname.AddSprintf("%s", GetSubStr(&res, 0, len+1));		
		query->SetAs(GetSubStr(&res, len+1, res.GetLength() - (len+1)));
		GetColumn(&funname, wyTrue);		
		return wyTrue;		
	}

	return wyFalse;
}

/*
this function will remove the comments in a query.	
extracing the comments from query , saving comment starting pos and comments in buffer.

Steps:
1) pattern for Matching entire comment .
2) replacing comment with singe horizantal space
3) trace the comment position
4) trimminng the spaces before and after comment  for easy comment pos trace. 
.
exceptional case: we didn`t handle condition comments,because we can n`t find
mysql version untill issuing query for mysql versions.
*/
void
SQLFormatter::GetQueryWtOutComments(wyString *commstr, wyString *uncommstr)
{	
	Comments   *cmntbuf = NULL;	
	wyString   res;
	wyString   pattern;
	wyInt32    matchret, findpos;	
	wyString   searchchar, temp;
	wyBool	   islefttrim = wyTrue;
	uncommstr->SetAs("");	

	while(1)
	{
		
		if(islefttrim)
			commstr->LTrim();
		islefttrim = wyTrue;
		if(commstr->GetLength() == 0)
			return ;

		pattern.SetAs(COMMENTS);
		matchret = MatchStringAndGetResult(commstr->GetString(), (wyChar*)pattern.GetString(), 
			PCRE_UTF8|PCRE_CASELESS, &res, wyFalse, wyTrue);

		if(matchret == NOTMATCHED)
		{
			uncommstr->Add(commstr->GetString());
			return;
		}

		//if conditional type comments
		if(commstr->Find("/*!", m_matchstartpos) == 0)
		{
			m_parseerr = wyTrue;
			return;
		}

		temp.SetAs(GetSubStr(commstr,0, m_matchstartpos));

		//if comments find inside special characters 
		if(IsPatternFindAsVarQualifier(&temp, &searchchar) == wyTrue)
		{
			if((findpos = commstr->Find(searchchar.GetString(), m_matchstartpos)) == -1)
			{
				m_parseerr = wyTrue;
				return ;
			}

			uncommstr->AddSprintf("%s", GetSubStr(commstr, 0, findpos + 1));						
			commstr->Sprintf("%s", GetSubStr(commstr, findpos +1, commstr->GetLength() - findpos));
			//Fixed: http://code.google.com/p/sqlyog/issues/detail?id=1923
			//Ltrim() should not be done since the comment is inside special characters and is a part of the query
			islefttrim = wyFalse;
			continue;
		}

		cmntbuf = (Comments*)malloc(sizeof(Comments));

		if(!cmntbuf)
			return ;

		cmntbuf->m_startpos = 0;
		cmntbuf->m_comment = NULL;

		m_pcmnts = (Comments**)realloc(m_pcmnts, sizeof(Comments*) *(m_cmntcnt + 1));	
		
		if(!m_pcmnts)
		{	
			free(cmntbuf);
			return ;
		}
		m_pcmnts[m_cmntcnt] = cmntbuf;
		m_pcmnts[m_cmntcnt]->m_startpos = 0 ;
		m_pcmnts[m_cmntcnt]->m_comment  = NULL ;
		m_pcmnts[m_cmntcnt]->m_leadspaces = 0;
		m_cmntcnt++;
		m_iscomment = wyTrue;	
		
		if(RemoveCommentsAndTracePOs(commstr, &res, uncommstr) == wyFalse)
		{
			m_parseerr = wyTrue;
			return;
		}
	}
}
//if searching pattern is used inside (""", "`", "'") characters as a variable name 
//or value qualifiers return true and retrieve type of character .
wyBool
SQLFormatter::IsPatternFindAsVarQualifier(wyString *query, wyString *chartype)
{
	wyInt32 count = 0;	
	char str[3] = {'`', '"', '\''};

	for(; count < 3; count++)
	{
		if(IsEvenBackticks(query->GetString(), str[count]) == wyFalse)
		{
			chartype->Sprintf("%c", str[count]);
			return wyTrue;
		}	
	}

	return wyFalse;
}

wyBool
SQLFormatter::RemoveCommentsAndTracePOs(wyString *commstr, wyString *res, 
										wyString *uncommstr)
{
	Comments   *cmntbuf = NULL;
	wyInt32    findpos;	
	wyString   comment;

	cmntbuf = m_pcmnts[m_cmntcnt - 1];

	//comment type /*comment1*/
	if(m_matchpatlen == 2 && commstr->GetCharAt(m_matchstartpos + 1) == '*')
	{
		findpos = res->Find("*/", 0);

		if(findpos == -1)
			return wyFalse;

		comment.Sprintf("/*%s*/", GetSubStr(res, 0, findpos));

		/*if query is exactly end with comments,  we stopped its parsing by setting query to empty*/
		commstr->Sprintf("%s", GetSubStr(res, findpos + 2, res->GetLength() - findpos - 2));		
	}
	else
	{
		comment.Sprintf("%s", GetSubStr(commstr, m_matchstartpos, m_matchpatlen));	
		commstr->Sprintf("%s", res->GetString());
	}

	cmntbuf->m_comment = (wyChar*)malloc(comment.GetLength() + 1);

	if(!cmntbuf->m_comment)
		return wyFalse;
    
	strcpy(cmntbuf->m_comment, comment.GetString());
	//removing spaces before and after comment
	m_uptomatch.RTrim();
	uncommstr->Add(m_uptomatch.GetString());
	//comment starting pos
	cmntbuf->m_startpos = uncommstr->GetLength();
	uncommstr->Add(" ");	

	return wyTrue;
}

//returns true if all columns are selected.
wyBool
SQLFormatter::IsAllColsSelected(wyString *query)
{
	wyInt32  matchret, trimcnt;
	wyString fromstr, pattern,res;
	wyInt32  spacetrimpos, nospaces;

	query->LTrim(&trimcnt);
	pattern.SetAs("^(\\*\\s*FROM[`\\(\\s])");

	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret  != MATCHED)
		return wyFalse;

	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	nospaces = 1 - trimcnt;
	TraceCommentPos(spacetrimpos, nospaces);
	m_queryFormate.AddSprintf("%s*%s", m_horspace, m_newline);	
	InsertSpaces(m_leadspaces);

	//erasing the * character in query
	query->Erase(0, 1);
	query->LTrim(&trimcnt);
	//adding one leadspaces before comment. if space is not exist before * character in a query
	spacetrimpos = m_querylen - query->GetLength() - trimcnt;
	nospaces = m_leadspaces + NEWLINELENGTH - trimcnt;
	TraceCommentPos(spacetrimpos, nospaces);
	m_queryFormate.AddSprintf("%s", GetSubStr(query, 0, 4));	
	InsertSpaces(SPACEAFTERKW);
	res.LTrim(&trimcnt);
	spacetrimpos = m_querylen - res.GetLength() - trimcnt;
	nospaces = SPACEAFTERKW - trimcnt;
	TraceCommentPos(spacetrimpos, nospaces);
	query->SetAs(res.GetString());
	return wyTrue;
}

wyInt32
SQLFormatter::FormateSelectStmt(wyString *query, wyBool isexplain)
{
	wyString  pattern, res;
	wyInt32   matchret , trimcnt;
	wyBool    parsecols = wyFalse;
	wyInt32   spacetrimpos, nospaces;

	SetSelectPattern(&pattern, MAIN_QUERY);
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED)
		return NOTMATCHED;
	
	//if explain is used with select stmt 
	if(isexplain == wyTrue)
	{
		//set the explain keyword to query formatter string
		m_queryFormate.Sprintf("%s", GetSubStr(query, 0, EXPLAINKWLENGTH));
		//erase the explian keyword
		query->Erase(0, EXPLAINKWLENGTH);
		m_prevstr.Erase(0, EXPLAINKWLENGTH);
		m_prevstr.LTrim();
		//adding one new line after explain stmt
		m_queryFormate.Add(m_newline);
		query->LTrim(&trimcnt);
        spacetrimpos = m_querylen - query->GetLength() - trimcnt;
		nospaces = NEWLINELENGTH - trimcnt;
		TraceCommentPos(spacetrimpos, nospaces);
	}
	
	m_queryFormate.AddSprintf("%s", GetSubStr(query, 0, 6));

	//prevstring last character is sep character('`', ', (, space) 
	//we need to erase these sep character for formatting select options
	m_prevstr.Erase(m_prevstr.GetLength() - 1, 1);	

	query->SetAs(res);	

	FormateSelectOptions(&m_prevstr);    
	
	if(IsAllColsSelected(query) == wyFalse)
		parsecols = wyTrue;	
	
	FormateSelectQuery(query, parsecols);

	return MATCHED;

}

//formate sub query.
wyBool
SQLFormatter::FormateIfSubQuery(wyString * query, wyInt32 leadspaces, wyBool wtnewline)
{
	wyString pattern, res, subquery;
	wyInt32  matchret;
	wyChar   *temp;	
	wyBool   parsecol = wyFalse, ismultipletables;
	wyInt32  sublen, trimcnt;
	wyInt32  mainldspaces, querylen, lbrkspaces = 0;
	wyInt32  lmaxcollen;
	wyString spacerefqry;
	wyInt32  spacetrimpos;
	wyInt32	 stmttype;
	
	SetSelectPattern(&pattern, SUB_QUERY);
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue,wyTrue);	

	if(matchret == NOTMATCHED)
		return wyFalse;	
	
	if(IsSubQueryCountExceed() == wyTrue)
		return wyTrue;
    
	//if we mesure leadspaces with newline procedure no need consideer formatting options
	//otherwise we need to consider line break options 	
	if(wtnewline == wyFalse)
		lbrkspaces = (m_opts->m_linebreak == 1?0:2);		
	
	//here we are saving all class level variables into local varaibles
	//if we continue  with out doing these we will loose all current query details.
	mainldspaces = m_leadspaces;
	m_leadspaces = leadspaces + lbrkspaces;

	stmttype = m_stmttype;
	m_stmttype = SELECT_STMT;
	ismultipletables = m_ismultipletables;
	m_ismultipletables = wyFalse;

	//it is erasing the separated characers that we used for matching
	m_prevstr.Erase(m_prevstr.GetLength() - 1, 1);	
	
	m_prevstr.Erase(0,1);
	//it will trim the space between '(' and select keyword
	m_prevstr.LTrim(&trimcnt);
	m_queryFormate.AddSprintf("(%s", GetSubStr(&m_prevstr, 0, 6));

	//here first argument is the pos of open paranthesis of subquery
	spacetrimpos = m_querylen - (query->GetLength() -1);
	TraceCommentPos(spacetrimpos,  -trimcnt);
	query->SetAs(res);	
	FormateSelectOptions(&m_prevstr);	
	
	if(IsAllColsSelected(query) == wyFalse)
		parsecol = wyTrue;

	temp = GetSubQuery(query);

	if(!temp)
	{
		m_parseerr = wyTrue;		
		return wyTrue;	
	}

    subquery.SetAs(temp);
	sublen = subquery.GetLength();
	
	querylen = m_querylen;
	lmaxcollen = m_maxcollen;

	/* we have to maintain parsed query length that will 
	abtain by substracting total length - rest string length 
	 .secone argument will give rest string length*/
	m_querylen = m_querylen - (query->GetLength()  - sublen);	

	spacerefqry.SetAs(m_spacerefquery);
	//if subquery is found  increment count by one
	m_subquerycnt += 1;
    
	FormateSelectQuery(&subquery, parsecol);

    //restoring the current query details back.
	m_spacerefquery.SetAs(spacerefqry);
	m_leadspaces = mainldspaces;
	m_querylen = querylen;
	m_maxcollen = lmaxcollen;

	m_ismultipletables = ismultipletables;
	m_stmttype = stmttype;

	//after subquery is formatted decrement count by one
	m_subquerycnt -= 1;
	//add subquery close paranthesis to the formatting query
	m_queryFormate.AddSprintf(")");	
	query->Sprintf("%s", GetSubStr(query, sublen + 1, (query->GetLength() - sublen - 1)));	

	return wyTrue;
}

//if subquery count exceed more than three return true. 
//we are supporting subquery levels up to three inner levels.
wyBool
SQLFormatter::IsSubQueryCountExceed()
{
	if(m_subquerycnt  >= 3)
	{
		m_parseerr = wyTrue;
		return wyTrue;
	}

	return wyFalse;
}

//formating the condtional sub query .
void
SQLFormatter::FormateCondSubQuery(wyString *query, wyString *matchstr)
{
	wyString subquery, spacerefqry;
	wyInt32  sublen;
	wyBool   parsecol = wyFalse, ismultipletables;
	wyChar   *temp;
	wyInt32  tleadspaces;	
	wyInt32  leadspaces, lmaxcollen, stmttype;	

	stmttype = m_stmttype;
	m_stmttype = SELECT_STMT;
	ismultipletables = m_ismultipletables;
	m_ismultipletables = wyFalse;

	//check query count exceed three inner subquery level or not
	if(IsSubQueryCountExceed() == wyTrue)
		return;

	leadspaces = m_queryFormate.GetLength() - 1 - NewLineLastOccurrence(m_queryFormate.GetString());
	
	//get the select key word from query to 
	//set the keyword with case(useful with Unchanged keyword option)to the formatted query.
	m_queryFormate.AddSprintf("%s", GetSubStr(matchstr,  0, 6));

	FormateSelectOptions(matchstr);

	tleadspaces = m_leadspaces;
	m_leadspaces = leadspaces;	
	
	if(IsAllColsSelected(query) == wyFalse)
		parsecol = wyTrue;	

	//before we allready erased ')' paranthesis
	//hence some times we may get its corresponding paranthesis or may not
	temp = GetSubQuery(query);
	
	//if corresponding paranthesis for this subquery is not found 
	//we can take entire string as subquery.
	if(!temp)
		subquery.SetAs(query->GetString());
	else
		subquery.SetAs(temp);


	//here we are saving all class level variables into local varaibles
	//if we continue  with out doing these we will loose all current query details.
	sublen = subquery.GetLength();
	spacerefqry.SetAs(m_spacerefquery);
	lmaxcollen = m_maxcollen;

	m_subquerycnt += 1;

	FormateSelectQuery(&subquery, parsecol);

	//restoring the current query details back.
	m_spacerefquery.SetAs(spacerefqry);	
	m_leadspaces = tleadspaces;  
	m_maxcollen = lmaxcollen;
	m_subquerycnt -= 1;

	m_ismultipletables = ismultipletables;
	m_stmttype = stmttype;
	
	query->Sprintf("%s",  GetSubStr(query, sublen + 1, query->GetLength() - sublen));		
}

//get the comment inserting position
wyInt32
SQLFormatter::GetCommentsInsertPos(wyInt32 cmntnumber)
{
	wyInt32 pos = 0;
	wyInt32 count;

	//comment position is obtaing by adding lead spaces of all its previos comments.
	for(count = 0; count <= cmntnumber; count++)
	{
		pos += m_pcmnts[count]->m_leadspaces ;

		if(count != 0)
			pos+= strlen(m_pcmnts[count - 1]->m_comment);
	}

	return pos;
}

void
SQLFormatter::FreeCommentBuffer()
{
	wyInt32  cmntcnt;
	Comments *cmntbuf = NULL;

	for(cmntcnt = 0; cmntcnt < m_cmntcnt; cmntcnt++)
	{
		cmntbuf = m_pcmnts[cmntcnt];

		if(cmntbuf)
		{
			if(cmntbuf->m_comment)
				free(cmntbuf->m_comment);
			
			free(cmntbuf);
		}		
	}

	if(m_pcmnts)
		free(m_pcmnts);
}

//adding comments to the formatted query
void
SQLFormatter::RestoreCommentsToQuery()
{
	wyString     tempstr, insstr;
	wyInt32      cmntcnt = 0, respos;
	wyInt32      cmntlen, prevcmntpos =0;

	for(;cmntcnt < m_cmntcnt; cmntcnt++)
	{
		respos = GetCommentsInsertPos(cmntcnt);
		respos += m_pcmnts[cmntcnt]->m_startpos;

		cmntlen = strlen(m_pcmnts[cmntcnt]->m_comment);

		if(respos < prevcmntpos)
			respos = prevcmntpos;

		//if comments ends with newline (--\s and #) and  one more new line comes after comment 
		//this case  new line is not required after comment.
		if(m_pcmnts[cmntcnt]->m_comment[cmntlen - 1] == '\n' && 
			m_queryFormate.GetCharAt(respos) == '\r')
		{
			m_pcmnts[cmntcnt]->m_comment[cmntlen -1] = '\0';

			if(m_pcmnts[cmntcnt]->m_comment[cmntlen - 2] == '\r')
				m_pcmnts[cmntcnt]->m_comment[cmntlen -2] = '\0';
			 
		}

		//inserting comment into  the formatted query
		if(m_queryFormate.Insert(respos, m_pcmnts[cmntcnt]->m_comment) == -1)
		{
			m_parseerr = wyTrue;
			return;
		}

		prevcmntpos = cmntlen + respos;
	}	
}

//formate the select query
wyInt32
SQLFormatter::FormateSelectQuery(wyString *query, wyBool parsecols)
{
	wyString  pattern, res;	
	wyInt32    ret, space; 
	
	/*if columns are specified in column selection list 
	we are parsing	these columns through the following function.*/
	if(parsecols == wyTrue)
	{
		if(ParseColumnLevel(query) == NOTMATCHED)
		{
			m_parseerr = wyTrue;
			return NOTMATCHED;
		}
	}

	query->LTrim(&space);

	if(IsEmptyQuery(query, space) == wyTrue)
		return MATCHED;

	ret = HandleTableReferences(query);
		
	return ret;
}

//formate the table if it comes inside paranthesis or as a subquery
void
SQLFormatter::HandleTableLevelSubquey(wyString *query, wyInt32 matchpatlen)
{
	wyInt32  leadspaces;		

	leadspaces = m_queryFormate.GetLength() - NewLineLastOccurrence(m_queryFormate.GetString());

	//here leadspaces is the last new line occurence pos in a formatted query.
	if(FormateIfSubQuery(query, leadspaces, wyTrue) == wyTrue)
	{
		if(m_parseerr == wyTrue)
			return;	

		//every derived table should have aliases
		MatchTableAliasPattern(query);
	}
	else
		TableEnclosedByParanthesis(query, matchpatlen);
}

//here matching  patterns are dumping options  patterns and table locking patterns
wyBool
SQLFormatter::MatchDumpAndSelModePatterns(wyString *query)
{
	wyString pattern, res;
	wyInt32  matchret, trimcnt;
	wyInt32  spacetrimpos, nospaces;

	query->LTrim(&trimcnt);

	if(IsEmptyQuery(query, trimcnt) == wyTrue)
		return wyTrue;	

	pattern.SetAs("^(PROCEDURE|INTO\\s+OUTFILE\\s+'|INTO\\s+DUMPFILE\\s+'|INTO\\s");
	pattern.Add("|FOR\\s+UPDATE|LOCK\\s+IN\\s+SHARE MODE)");

	matchret = MatchStringAndGetResult(query->GetString(),(wyChar*)pattern.GetString(), PCRE_COMPLIE_OPTIONS, &res);

	if(matchret == MATCHED)
	{
		trimcnt = IsSpaceSepChar(query->GetLength());
		spacetrimpos = m_querylen - query->GetLength() - trimcnt;
		nospaces = NEWLINELENGTH - trimcnt;
		TraceCommentPos(spacetrimpos, nospaces);
		m_queryFormate.AddSprintf("%s%s", m_newline, query->GetString());
		query->SetAs("");
		return wyTrue;
	}

	return wyFalse;
}

/*here we are finding which pattern is used
after table name and formatting the query according to that pattern.

patterns possibulities after table name.
1) comma 
2) where , group by , order by , having , limit , procedure , dump options
3) joins. 
*/
wyInt32
SQLFormatter::FormateSelTableReferences(wyString *query, wyBool indexsyntax)
{
	wyInt32		ret;

	if((ret = TableListSepWithComma(query)) != CONTINUE_PARSE)
			return ret;
    
	//if it is update stmt, then we are looking for SET clause
	if(m_stmttype == UPDATE_STMT)
	{
		if(HandleSetClause(query) == wyTrue || m_parseerr == wyTrue)
			return MATCHED;

	}
	else if(m_stmttype == DELETE_STMT)
	{
		//if it is delete stmt, and multiple table syntax then we will look for USING clause
		if(m_isusingpresent == wyTrue && m_ismultipletables == wyTrue) 
		{
			//if USING clause is present in Syntax we need to search for USING
			//if it is single table or table is present in b/w DELTE & FROM then there is no USING clause.
			if(HandleUsingClauseForDelete(query) == MATCHED)
				return MATCHED;
		}//If it is delete stmt and single table syntax & multiple table syntax without USING clause we r looking for where,order by &limit
		else if(SelectionLimitClauses(query) == wyTrue || m_parseerr == wyTrue)
			return MATCHED;
	
	}
	//if it is a select stmt then we are searching for where, order by, group by etc.
	else if(SelectionLimitClauses(query) == wyTrue || m_parseerr == wyTrue)
			return MATCHED;
	
	//if table joins with another table
	if(FormateTableJoins(query) == wyTrue)
	{
		if((ret = TableListSepWithComma(query)) != CONTINUE_PARSE)
			return ret;
	}
	
	if(indexsyntax == wyTrue)
		HandleIndexHintSyntax(query);

	if(m_stmttype == SELECT_STMT)
	{
		if(MatchDumpAndSelModePatterns(query) == wyTrue)
			return MATCHED;
	}

	return CONTINUE_PARSE;
}

//if tables are separated with comma
wyInt32 
SQLFormatter::TableListSepWithComma(wyString *query)
{
	wyInt32 space;

	query->LTrim(&space);

	if(IsEmptyQuery(query, space) == wyTrue || m_parseerr == wyTrue)
		return MATCHED;
	
	if(query->GetCharAt(0) == ',')
	{
		space = IsSpaceSepChar(query->GetLength());
		CommaSeparator(query, space);
		m_ismultipletables = wyTrue;
		return PARSETABLE;
	}
    
	return CONTINUE_PARSE;
}

//return true if query is empty.
wyBool
SQLFormatter::IsEmptyQuery(wyString *query, wyInt32 space)
{
	wyInt32  spacetrimpos;

	if(query->GetLength() == 0)
	{
		//here in commant tracing comments positions are moving one pos back if trim is one
		spacetrimpos = m_querylen - query->GetLength() - space;
		TraceCommentPos(spacetrimpos, -space);
		return wyTrue;
	}

	return wyFalse;
}

//if space is used at the specified pos it returns one otherwise it returns 0
wyInt32
SQLFormatter::IsSpaceSepChar(wyInt32 findpos)
{
	return((m_spacerefquery.GetCharAt(m_querylen -findpos- 1) == ' ')?1:0);
}

//if more tables are used in the table list with comma separation.
wyInt32
SQLFormatter::CommaSeparator(wyString *query, wyInt32 bfrecomma)
{
	wyInt32 aftercomma = 0, curquerypos;
	wyInt32 spacetrimpos, nospaces;

	curquerypos = m_querylen - query->GetLength();
	//erasing comma character in a query
	query->Erase(0, 1);
	query->LTrim(&aftercomma);

	if(m_opts->m_stacked == wyTrue)
        LineBreaksWithComma(curquerypos, bfrecomma, aftercomma);
	else
	{
		spacetrimpos = curquerypos - bfrecomma;
		TraceCommentPos(spacetrimpos, -bfrecomma);
		//in non stack mode one space is adding after comma
		m_queryFormate.AddSprintf(",%s", m_horspace);
		spacetrimpos = curquerypos + 1;
		nospaces = 1 - aftercomma;
		TraceCommentPos(spacetrimpos, nospaces);
	}

	return PARSETABLE;
}

wyInt32 
SQLFormatter::MatchStringAndGetResult(const wyChar*subject, wyChar *pattern, wyInt32 pcrecompileopts, 
									 wyString *res, wyBool addsepchars, wyBool prevstr, wyBool getsubstr)
{	
	wyInt32   regexret = 0;				 // pcre_exec return values.
	wyInt32   ovector[OVECCOUNT];       // output vector for substring information 
	
	res->SetAs("");
	SetPcreVarsToDefault();

	if(strlen(subject) == 0)
		return NOTMATCHED;

	regexret = MatchStringPattern(subject, pattern, ovector, pcrecompileopts);

	if(regexret < 0)
	{
		if(regexret != -1)
			m_parseerr = wyTrue;

		return NOTMATCHED;
	} 

	//if we want to know only query is matched to the specific patterns or not . 
	if(getsubstr == wyFalse)
		return MATCHED;

	GetSubStrings(subject, res, ovector, prevstr, addsepchars);
	return MATCHED;
}

void
SQLFormatter::SetPcreVarsToDefault()
{
	m_uptomatch.SetAs("");
	m_matchpatlen = 0;
	m_matchstartpos = 0;
	m_prevstr.SetAs("");
	m_uptomatch.SetAs("");
}

/* get substrings. if bfrematchflag is true get string upto match
if flag is true get strings from match position.
*/
void
SQLFormatter::GetSubStrings(const wyChar*subject, wyString *res, wyInt32 *ovector,wyBool prevstr, wyBool addsepchars)
{
	wyString tempstr;
	
	m_matchpatlen = ovector[1] - ovector[0];
	tempstr.SetAs(subject);

	m_matchstartpos = ovector[0];
	m_matchendpos = ovector[1];

	//string before match
	if(prevstr == wyTrue)
		m_prevstr.SetAs(GetSubStr(&tempstr, 0, m_matchendpos));	

	if(m_matchstartpos > 0)
		m_uptomatch.Sprintf("%s", GetSubStr(&tempstr, 0, m_matchstartpos));		

    //here rest string contain sep character also
	if(addsepchars == wyTrue)
	{
		res->SetAs(GetSubStr(&tempstr, m_matchendpos - 1, strlen(subject)- m_matchendpos + 1));		
		return;
	}	
    	
	res->SetAs(GetSubStr(&tempstr, m_matchendpos, strlen(subject)- m_matchendpos)); 
}

//freeing the memory allocated for columns
void
SQLFormatter::FreeColumnList(ColumnList **pcollist, wyInt32 colcnt)
{
	wyInt32 cnt = 0;

	if(m_opts->m_stacked == wyFalse)
		return;

	for( ; (cnt < colcnt && pcollist[cnt]) ;cnt++)
	{
		if(pcollist[cnt]->m_col)
			free(pcollist[cnt]->m_col);

		if(pcollist[cnt]->m_alias)
			free(pcollist[cnt]->m_alias);

		if(pcollist[cnt]->m_askw)
			free(pcollist[cnt]->m_askw);

		free(pcollist[cnt]);
	}

	if(pcollist)
		free(pcollist);
}

void 
SQLFormatter::FormateSelectOptions(wyString *query )
{
	wyString  selectoptions;
	
	if(query->GetLength() > (SELECTKWLENGTH + 1))
	{
		//If any options present in select stmt then after select we are adding spaces then we are adding that options		
		selectoptions.SetAs(GetSubStr(query, 6, query->GetLength() - 6));		
		m_queryFormate.AddSprintf("%s", selectoptions.GetString());
	}
}

//if subquery is used as a column. 
wyBool 
SQLFormatter::HandleColumnSubQuery(wyString *query)
{
	ColumnList *ptr;
	wyString   mainquery, spaceref;
	wyInt32    tracecmntno;

	if(m_opts->m_stacked == wyFalse)
		return HandleColSubQueryInNonStackMode(query);

	ptr = m_collist;
	mainquery.SetAs(m_queryFormate.GetString());
	m_queryFormate.SetAs("");	

	tracecmntno = m_trackcmntno;

	//normal lead spaces before column and one additional spaces for subquery open paranthesis
	if(FormateIfSubQuery(query, m_leadspaces + m_opts->m_spaceindentation +1, wyFalse) == wyTrue)
	{	
		if(m_parseerr == wyTrue)
			return wyTrue;
	
		m_collist = ptr;
		m_collist->m_col = (wyChar*)malloc(m_queryFormate .GetLength() + 1);
		
		if(m_collist->m_col)
			strcpy(m_collist->m_col, (wyChar*)m_queryFormate.GetString());

		m_queryFormate.SetAs(mainquery.GetString());
		//here we are restoring current trace comment no to previous value 
		//other wise comment pos may change.
		m_trackcmntno = tracecmntno;
		return wyTrue;
	}

	m_queryFormate.SetAs(mainquery.GetString());
	return wyFalse;	
}

//if NonStackMode is selected for listing columns
wyBool 
SQLFormatter::HandleColSubQueryInNonStackMode(wyString *query)
{
	wyInt32 leadspaces;

	leadspaces = m_queryFormate.GetLength() - NewLineLastOccurrence(m_queryFormate.GetString());		
	return FormateIfSubQuery(query, leadspaces + 1, wyTrue);		
}

//handle the column if it comes inside paranthesis
void 
SQLFormatter::ColumnEnclosedByParanthesis(wyString *query)				
{
	wyString subquery;
	wyInt32  len;

	query->Erase(0,1);

	len = GetStrLenWithInParanthesis(query->GetString(), wyTrue);

	//if corresponding paranthesis is not found.
	if(len == -1)
	{
		m_parseerr = wyTrue;
		return ;	
	}

	subquery.Sprintf("(%s", GetSubStr(query, 0, len+1)); 
	len = subquery.GetLength();

	if(m_opts->m_stacked == wyTrue)
	{
		m_collist->m_col = (wyChar*)malloc(subquery.GetLength() + 1);

		if(m_collist->m_col)
			strcpy(m_collist->m_col, (wyChar*)subquery.GetString());		
	}
	else
        m_queryFormate.Add(subquery.GetString());

	query->Sprintf("%s",  GetSubStr(query, len - 1, (query->GetLength() - (len - 1))) );    	
}

//get substring 
wyChar*
SQLFormatter::GetSubStr(wyString *query, wyInt32 startpos, wyInt32 endpos)
{
	
	if(startpos < 0 || endpos < 0)
	{
		m_parseerr = wyTrue;
		return("");
	}

	if(startpos >= query->GetLength())
		return("");		

	//return empty string if SubStr return NULL value
	return query->Substr(startpos, endpos);
}

//it iwll format the update stmt
wyInt32
SQLFormatter::FormateUpdateStmt(wyString *query)
{
	wyString  pattern, res;
	wyInt32   matchret ;
	wyString  fromstr, selectstr, selectoptions;
	wyInt32   space, querylen, noofspaces;

	m_stmttype = UPDATE_STMT; 

	pattern.SetAs("^UPDATE(\\s+LOW_PRIORITY)?(\\s+IGNORE)?[\\s`\\(]");
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED)
		return NOTMATCHED;
	
	m_queryFormate.AddSprintf("%s", GetSubStr(query, 0, 6));
	m_prevstr.Strip(1);

	query->SetAs(res);

	FormateSelectOptions(&m_prevstr);

	query->LTrim(&space);
	InsertSpaces(SPACEAFTERKW);
	querylen = m_querylen - query->GetLength();
	noofspaces = SPACEAFTERKW - space;
	TraceCommentPos(querylen, noofspaces);
	
	HandleTableReferences(query);

	return MATCHED;
}

wyBool
SQLFormatter::HandleSetClause(wyString *query)
{
	wyString	res;
	wyInt32		matchret;
	wyString	pattern;
	wyInt32		querylen, space,nospaces, spacetrimpos;
			
	pattern.SetAs("^SET[`\\(\\s]");
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED || res.GetLength() == 0)
		return wyFalse;

	m_prevstr.Strip(1);

    //adding one newline before "SET" clause.
	space = IsSpaceSepChar(query->GetLength());
	spacetrimpos = m_querylen - query->GetLength()- space;
	nospaces = m_leadspaces + NEWLINELENGTH - space;
	InsertSpaces(m_leadspaces, wyTrue);
	TraceCommentPos(spacetrimpos, nospaces);
	
	m_queryFormate.AddSprintf("%s", m_prevstr.GetString());
	
	// adding one space after "SET" clause and adjust that space with comment position.
	InsertSpaces(SPACEAFTERKW);
	querylen = m_querylen - res.GetLength();
	TraceCommentPos(querylen, SPACEAFTERKW);
	query->SetAs(res);

	FormateSetClause(query);

	return wyTrue;
}

wyInt32
SQLFormatter::FormateSetClause(wyString *query)
{
	wyString	pattern, res, condexp, tempstr;
	wyInt32		space;
		
	while(1)
	{
		query->LTrim(&space);

		if(IsEmptyQuery(query, space) == wyTrue)
			return MATCHED;

		TraceCommentPos(m_querylen - query->GetLength(), -space);

		//matching column name and "=" 
		if(MatchColumnPattern(query) == wyFalse)
		{	
			m_parseerr = wyTrue;
			return NOTMATCHED;
		}

		//this is used for handling expression after "=" sign
		HandleColumnExpForUpdate(query);
			
	}

	return MATCHED;
}

wyInt32 
SQLFormatter::HandleTableReferences(wyString *query)
{
	wyBool		indexsyntax = wyFalse;
	wyInt32		ret; 
	
	while(1)
	{		
		//match tables (subquery or table with in paranthesis)
		if(query->GetCharAt(0) == '(')
		{
			//here subquery can be used as a table
			HandleTableLevelSubquey(query, 0);			
		}
		else
		{
			MatchTablePattern(query);			
			m_queryFormate.AddSprintf("%s", m_prevstr.GetString());

			if((ret = FormateSelTableReferences(query, wyTrue)) == PARSETABLE)
				continue;
			
			if(ret != CONTINUE_PARSE)
				return ret;

			indexsyntax = wyTrue;
			MatchTableAliasPattern(query);	
		}

		if(m_parseerr == wyTrue)
			return NOTMATCHED;

		if((ret = FormateSelTableReferences(query, indexsyntax)) == PARSETABLE)
			continue;
		
		if(ret == CONTINUE_PARSE)
			m_parseerr = wyTrue;
		
		return ret;
	}
}

void
SQLFormatter::HandleColumnExpForUpdate(wyString *query)
{
	wyString	pattern, res, condexp;
	wyInt32		matchret, pos, leadspaces;
	wyString	str;
	wyString	searchchar;

	condexp.SetAs("");
	
	//we are checking for ")",",","where" etc
	pattern.SetAs("\\(|,|WHERE[`\\(\\s]");
    if(m_ismultipletables == wyFalse)//if mutipletables then order by,limit will not come in update stmt
		pattern.Add("|ORDER\\s+BY[`\\)\\s]|LIMIT\\s");
		
	while(1)
	{
		leadspaces = m_leadspaces + m_opts->m_spaceindentation + 2;
		matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
				PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

		if(matchret == NOTMATCHED)
		{ 
			condexp.AddSprintf("%s", query->GetString());
			HandleCondExp(&condexp, 0, leadspaces);
			query->SetAs(res);
			return ;
		}

		str.SetAs(GetSubStr(query, 0, m_matchendpos - 1));
		//whether matched pattern is used inside backticks or not.
		//if pattern is used inside backticks,  skip all characters up to next backtick and 
		//continue parsing with the rest string.
		if(IsPatternFindAsVarQualifier(&str, &searchchar) == wyTrue)
		{
			if((pos = res.Find(searchchar.GetString(), 0)) == -1 )
			{
				m_parseerr = wyTrue;
				return ;
			}

			condexp.AddSprintf("%s", query->Substr(0, m_matchendpos + pos));
			res.Erase(0, pos + 1);
			query->SetAs(res);
			continue;
		}

		if(PatternMatchedInColumnExp(query, &res, &condexp, leadspaces) == SUCCESS)
			return ;
	}
}

wyInt32
SQLFormatter::PatternMatchedInColumnExp(wyString *query, wyString *res, wyString *condexp,  wyInt32 leadspaces)
{
	wyString	tempstr;
	wyInt32		condexplen, len;
	wyInt32		reslen, spacetrimpos, nospaces;
	
	//if it is comma or parenthesis
	if(m_matchpatlen == 1)
	{
		tempstr.Sprintf("%s", query->Substr(m_matchstartpos, m_matchpatlen));
	}
	else//if it is "where" , "order by"
		tempstr.Sprintf("%s", query->Substr(m_matchstartpos, m_matchpatlen - 1));
	
	//If subquery or function or string within parenthesis
	if(tempstr.FindIWithReverse("(", 0, wyTrue) == 0)
	{
		res->Erase(0, 1);
		len = GetStrLenWithInParanthesis(res->GetString(), wyTrue);

		if(len == -1)
		{			
			m_parseerr = wyTrue;
			return wyTrue;
		}
			
		condexp->AddSprintf("%s", GetSubStr(query, 0, m_matchstartpos + m_matchpatlen + len + 1));
		query->Sprintf("%s", GetSubStr(res, len + 1, res->GetLength() - len - 1));		
		return CONTINUE_PARSE;
	}
	
	condexplen = query->GetLength() - (res->GetLength() + m_matchpatlen -1 );
	condexp->AddSprintf("%s", GetSubStr(query, 0, condexplen));
	reslen = res->GetLength();//if it is comma then no need to add 
	
	if(m_matchpatlen != 1)//if it is where /order by / limit then we need to add this length also.
		reslen = reslen + tempstr.GetLength();

	HandleCondExp(condexp, reslen, leadspaces);
	m_queryFormate.RTrim();
	
	if(tempstr.GetCharAt(0) == ',')
	{	
		query->Sprintf("%s", res->GetString());
		//Here we are sending 0 for beforecomma because this space we are handling in HandleCondExp() function.
		//we will match till comma and we are passing this to HandleCondExp()
		CommaSeparator(query, 0);
		return SUCCESS;
	}
	
	//trace comment pos before selection limit caluses.		
	//in cond exp we have trimmed one space hence here we need to add one space always
	spacetrimpos = m_querylen - res->GetLength() - tempstr.GetLength();
	nospaces = m_leadspaces + NEWLINELENGTH;
	TraceCommentPos(spacetrimpos, nospaces);
	
	FormateSelectionLimitClauses(res, &tempstr);
	
	query->SetAs("");
	return SUCCESS;
}

void
SQLFormatter::GetColumnPatternWithBacticks(wyString *pattern)
{
	/*columns can use in three ways 1)along with db or along with table or directly column.
	 if columns in a query comes any one of above following combination like `db`...., `table`.., `col`.
	By using the the following regex patterrns we can match any of above type of columns.	
	 --last two patterns are used for  matching the selected values in the column list like
		 EX:1)SELECT "VALUES" 
			2)SELECT 'VALUES'.*/
	if(m_stmttype == SELECT_STMT)
	{
		pattern->SetAs("^`[^`]+`\\s*\\.\\s*((`[^`]+`|[^`|^\\s|^,|^\"|^'|^\\.]+)(\\s*\\.\\s*))?");
		pattern->Add("(`[^`]+`|[^`|^\\s|^,|^\"|^'|^\\.]+)|(^`[^`]+`|^'[^']*'|^\"[^\"]*\")");
	}
	else if(m_stmttype == UPDATE_STMT)
	{
		pattern->SetAs("^`[^`]+`\\s*\\.\\s*((`[^`]+`|[^`|^\\s|^=|^\\.]+)(\\s*\\.\\s*))?");
		pattern->Add("(`[^`]+`|[^`|^\\s|^=|^\\.]+)(\\s*=)|^`[^`]+`\\s*=");
	}
	else
	{
		pattern->SetAs("^`[^`]+`\\s*\\.\\s*((`[^`]+`|[^`|^\\s|^,|^\\.|^=|^\\)]+)(\\s*\\.\\s*))?");
		pattern->Add("(`[^`]+`|[^`|^\\s|^,|^\\)|^=|^\\.]+)|(^`[^`]+`)");
	}
}

void
SQLFormatter::GetColumnPatternWithOutBacticks(wyString *pattern)
{
	//if columns in a query comes any one of following formates db...., table.., col.
	// the following regex patterrn is used to match.
	if(m_stmttype == SELECT_STMT)
	{
		pattern->SetAs("^`[^`|^\\s|^,|^\"|^\'|^\\.]+\\s*\\.\\s*");
		pattern->Add("((`[^`]+`|[^`|^\\s|^,|^\"|^\\.]+)(\\s*\\.\\s*))?");
		pattern->Add("(`[^`]+`|[^`|^\\s|^,|^\"|^\'|^\\.]+)|(^`[^`|^\\s|^,|^\"|^\\.|^\']+)");

	}
	else if(m_stmttype == UPDATE_STMT)
	{
		pattern->SetAs("^`[^`|^\\s|^=|^\\.]+\\s*\\.\\s*");
		pattern->Add("((`[^`]+`|[^`|^\\s|^\\.|^=]+)(\\s*\\.\\s*))?");
		pattern->Add("(`[^`]+`|[^`|^\\s|^=|^\\.]+)(\\s*=)|(^`[^`|^\\s|^=|^\\.]+)(\\s*=)");
	}
	else
	{  //For insert stmt
		pattern->SetAs("^`[^`|^\\s|^,|^=|^\\)|^\\.]+\\s*\\.\\s*");
		pattern->Add("((`[^`]+`|[^`|^\\s|^,|^\\)|^=|^\\.]+)(\\s*\\.\\s*))?");
		pattern->Add("(`[^`]+`|[^`|^\\s|^,|^\\)|^=|^\\.]+)|(^`[^`|^\\s|^,|^\\)|^\\.|^=]+)");
	}
}


wyInt32
SQLFormatter::FormateDeleteStmt(wyString *query)
{
	wyString  pattern, res;
	wyInt32   matchret ;
	wyString  fromstr, selectstr, selectoptions;
	
	m_stmttype = DELETE_STMT; 

	pattern.SetAs("^DELETE(\\s+LOW_PRIORITY)?(\\s+QUICK)?(\\s+IGNORE)?[\\s`\\(]");
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(), 
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);
	
	if(matchret == NOTMATCHED)
		return NOTMATCHED;
	
	m_queryFormate.AddSprintf("%s", GetSubStr(query, 0, 6));
	m_prevstr.Strip(1);

	query->SetAs(res);

	FormateSelectOptions(&m_prevstr);

	FormateDeleteQuery(query);

	return MATCHED;
}

wyInt32
SQLFormatter::FormateDeleteQuery(wyString *query)
{	
	wyInt32 querylen, space, ret;

	//if it is single table syntax or multiple table with using clause then After DELETE keyword FROM will come
	//So first we are searching for FROM clause.
	if(HandleFromClause(query) == MATCHED)
		return MATCHED;

	InsertSpaces(SPACEAFTERKW);
	querylen = m_querylen - query->GetLength();
	TraceCommentPos(querylen, SPACEAFTERKW);
	
	//If it is multiple table syntax then table names will come in b/w DELETE and FROM clause.
	while(1)
	{
		if(query->GetCharAt(0) == '(')
		{
			//if subquery comes b/w DELETE & FROM it is not a valid query.
			//Only table names are allowed in b/w DELETE & FROM 
			m_parseerr = wyTrue;
			return NOTMATCHED;
		}

		MatchTablePattern(query);

		if(m_parseerr == wyTrue)
			return NOTMATCHED;

		m_queryFormate.AddSprintf("%s", m_prevstr.GetString());

		//if table names are present in b/w DELETE & FROM, then it is multiple table delete stmt.
		//if table is present in b/w DELETE & FROM then there is no USING clause.
		m_isusingpresent = wyFalse;

		if((TableListSepWithComma(query)) == PARSETABLE)
			continue;
	       
		space = IsSpaceSepChar(query->GetLength());
		TraceCommentPos(m_querylen - query->GetLength() - space, -space);

		if((ret = HandleFromClause(query)) == MATCHED)
			return MATCHED;

		if(ret == NOTMATCHED)
			m_parseerr = wyTrue;
	}
}

wyInt32
SQLFormatter::HandleFromClause(wyString *query)
{
	wyString	pattern, res;
	wyInt32		matchret, space, querylen, noofspaces;	

	query->LTrim(&space);
	if(IsEmptyQuery(query, space) == wyTrue)
		return MATCHED;

	querylen = m_querylen - query->GetLength();
	TraceCommentPos(querylen, -space);

	pattern.SetAs("^FROM[`\\(\\s]");
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

	if(matchret == MATCHED)
	{
		m_prevstr.Strip(1);
        InsertSpaces(m_leadspaces, wyTrue);
		TraceCommentPos(querylen, m_leadspaces + NEWLINELENGTH);
		m_queryFormate.AddSprintf("%s", m_prevstr.GetString());
		query->Sprintf("%s", res.GetString());

		query->LTrim(&space);
		InsertSpaces(SPACEAFTERKW);
		querylen = m_querylen - query->GetLength();
		noofspaces = SPACEAFTERKW - space;
		
		TraceCommentPos(querylen, noofspaces);
		HandleTableReferences(query);
		return MATCHED;
	}
	return NOTMATCHED;
}

wyInt32
SQLFormatter::HandleUsingClauseForDelete(wyString *query)
{
	wyString	pattern, res;
	wyInt32		matchret, space, querylen, noofspaces;

	pattern.SetAs("^USING[`\\(\\s]");
	matchret = MatchStringAndGetResult(query->GetString(), (wyChar*)pattern.GetString(),
		PCRE_COMPLIE_OPTIONS, &res, wyTrue, wyTrue);

	m_prevstr.Strip(1);

	if(matchret == MATCHED)
	{
		//if USING found, then we are setting m_isusingpresent to wyFalse. 
		//if m_isusingfound is wyTrue then only we will search for USING.
		m_isusingpresent = wyFalse;

		InsertSpaces(m_leadspaces, wyTrue);
		space = IsSpaceSepChar(query->GetLength());
		querylen = m_querylen - query->GetLength();
		TraceCommentPos(querylen, m_leadspaces + NEWLINELENGTH - space);

		m_queryFormate.AddSprintf("%s", m_prevstr.GetString());
		query->Sprintf("%s", res.GetString());

		query->LTrim(&space);
		InsertSpaces(SPACEAFTERKW);
		querylen = m_querylen - query->GetLength();
		noofspaces = SPACEAFTERKW - space;
		TraceCommentPos(querylen, noofspaces);

		HandleTableReferences(query);
		return MATCHED;
	}

	return NOTMATCHED;
}
