/*
 *  $Id$
 *
 *  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
 *  project.
 *
 *  Copyright (C) 1998-2008 OpenLink Software
 *
 *  This project is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; only version 2 of the License, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

package virtuoso.jena.driver;

import java.sql.*;
import java.io.*;
import java.util.*;
import java.util.Iterator;

import virtuoso.jdbc3.*;

import java.util.Iterator;
import java.util.Map;

import com.hp.hpl.jena.shared.*;
import com.hp.hpl.jena.shared.PrefixMapping;
import com.hp.hpl.jena.shared.impl.PrefixMappingImpl;

public class VirtPrefixMapping extends PrefixMappingImpl {

	protected VirtGraph m_graph = null;
	
	/**
	 * Constructor for a persistent prefix mapping.
	 * 
	 */
	public VirtPrefixMapping( VirtGraph graph) 
	{
	  super();
	  m_graph = graph;
		
	  // Populate the prefix map using data from the 
	  // persistent graph properties
	  Connection conn = m_graph.getConnection();
	  String query = "DB.DBA.XML_SELECT_ALL_NS_DECLS (3)";
	  try {
	    Statement stmt = conn.createStatement();
	    VirtuosoResultSet rs = (VirtuosoResultSet) stmt.executeQuery(query);

  	    while (rs.next()) {
	      String prefix = rs.getString(1);
	      String uri = rs.getString(2);
	      if (uri != null && uri != null)
	        super.setNsPrefix(prefix, uri);
	    }
	  } catch (Exception e) {
	     throw new JenaException(e.toString());
	  } 
	}

        public PrefixMapping removeNsPrefix( String prefix )
        {
	  Connection conn = m_graph.getConnection();
	  String query = "DB.DBA.XML_REMOVE_NS_BY_PREFIX('" + prefix + "', 1)";
          super.removeNsPrefix( prefix );

	  try {
	    Statement stmt = conn.createStatement();
	    stmt.execute(query);
	  } catch (Exception e) {
	     throw new JenaException(e.toString());
	  } 

          return this;
        }
    
	/* (non-Javadoc)
	 * Override the default implementation so we can catch the write operation
	 * and update the persistent store.
	 * @see com.hp.hpl.jena.shared.PrefixMapping#setNsPrefix(java.lang.String, java.lang.String)
	 */
	public PrefixMapping setNsPrefix(String prefix, String uri) 
	{
	  super.setNsPrefix(prefix, uri);

	  Connection conn = m_graph.getConnection();
	  String query = "DB.DBA.XML_SET_NS_DECL('" + prefix + "','" + uri + "', 1)";
		
	  // All went well, so persist the prefix by adding it to the graph properties
	  // (the addPrefix call will overwrite any existing mapping with the same prefix
	  // so it matches the behaviour of the prefixMappingImpl).
	  try {
	    Statement stmt = conn.createStatement();
	    stmt.execute(query);
	  } catch (Exception e) {
	     throw new JenaException(e.toString());
	  } 
          return this;
	}

	public PrefixMapping setNsPrefixes(PrefixMapping other) 
	{
	  return setNsPrefixes(other.getNsPrefixMap());
	}

	public PrefixMapping setNsPrefixes(Map other) 
	{
          checkUnlocked();
	  Iterator it = other.entrySet().iterator();
	  while (it.hasNext()) {
	 	Map.Entry e = (Map.Entry) it.next();
		setNsPrefix((String) e.getKey(), (String) e.getValue());
	  }
          return this;
	}
}
