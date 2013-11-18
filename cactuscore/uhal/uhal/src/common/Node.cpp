/*
---------------------------------------------------------------------------

    This file is part of uHAL.

    uHAL is a hardware access library and programming framework
    originally developed for upgrades of the Level-1 trigger of the CMS
    experiment at CERN.

    uHAL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uHAL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uHAL.  If not, see <http://www.gnu.org/licenses/>.


      Andrew Rose, Imperial College, London
      email: awr01 <AT> imperial.ac.uk

      Marc Magrans de Abril, CERN
      email: marc.magrans.de.abril <AT> cern.ch

---------------------------------------------------------------------------
*/

#include "uhal/Node.hpp"

#include "uhal/HwInterface.hpp"
#include "uhal/ValMem.hpp"
#include "uhal/NodeTreeBuilder.hpp"
#include "uhal/Utilities.hpp"

#include "uhal/log/log.hpp"

#include <boost/regex.hpp>

#include <iomanip>

#include "uhal/log/BacktraceSymbols.hpp"

#ifdef __GNUG__
#include <cxxabi.h>
#endif


/**
	The log_inserter function to add the node object to a log entry
	@param aNode a node to format and print to log
	@param aStr a stream to which to append the formatted node
	@return a reference to the stream object for chaining stream calls
*/
std::ostream& operator<< ( std::ostream& aStr ,  const uhal::Node& aNode )
{
  aNode.stream ( aStr );
  return aStr;
}






namespace uhal
{





  Node::Node ( )  :
    mHw ( NULL ),
    mUid ( "" ),
    mPartialAddr ( 0x00000000 ),
    mAddr ( 0x00000000 ),
    mMask ( defs::NOMASK ),
    mPermission ( defs::READWRITE ),
    mMode ( defs::HIERARCHICAL ),
    mSize ( 0x00000001 ),
    mTags ( "" ),
    mDescription ( "" ),
    mModule ( "" ),
    mParameters ( ),
    mChildren ( ),
    mChildrenMap ( )
  {
  }


  Node::Node ( const Node& aNode )  :
    mHw ( aNode.mHw ),
    mUid ( aNode.mUid ),
    mPartialAddr ( aNode.mPartialAddr ),
    mAddr ( aNode.mAddr ),
    mMask ( aNode.mMask ),
    mPermission ( aNode.mPermission ),
    mMode ( aNode.mMode ),
    mSize ( aNode.mSize ),
    mTags ( aNode.mTags ),
    mDescription ( aNode.mDescription ),
    mModule ( aNode.mModule ),
    mParameters ( aNode.mParameters ),
    mChildren ( ),
    mChildrenMap ( )
  {
    for ( std::deque< Node* >::const_iterator lIt = aNode.mChildren.begin(); lIt != aNode.mChildren.end(); ++lIt )
    {
      mChildren.push_back ( ( **lIt ).clone() );
    }

    for ( std::deque< Node* >::iterator lIt = mChildren.begin(); lIt != mChildren.end(); ++lIt )
    {
      mChildrenMap.insert ( std::make_pair ( ( **lIt ).mUid , *lIt ) );

      for ( boost::unordered_map< std::string , Node* >::iterator lSubMapIt = ( **lIt ).mChildrenMap.begin() ; lSubMapIt != ( **lIt ).mChildrenMap.end() ; ++lSubMapIt )
      {
        mChildrenMap.insert ( std::make_pair ( ( ( **lIt ).mUid ) +'.'+ ( lSubMapIt->first ) , lSubMapIt->second ) );
      }
    }
  }




  Node& Node::operator= ( const Node& aNode )
  {
    mHw = aNode.mHw;
    mUid = aNode.mUid ;
    mPartialAddr = aNode.mPartialAddr;
    mAddr = aNode.mAddr;
    mMask = aNode.mMask;
    mPermission = aNode.mPermission;
    mMode = aNode.mMode;
    mSize = aNode.mSize;
    mTags = aNode.mTags;
    mDescription = aNode.mDescription;
    mModule = aNode.mModule;
    mParameters = aNode.mParameters;

    for ( std::deque< Node* >::iterator lIt = mChildren.begin(); lIt != mChildren.end(); ++lIt )
    {
      if ( *lIt )
      {
        delete ( *lIt );
        ( *lIt ) = NULL;
      }
    }

    mChildren.clear();
    mChildrenMap.clear();

    for ( std::deque< Node* >::const_iterator lIt = aNode.mChildren.begin(); lIt != aNode.mChildren.end(); ++lIt )
    {
      mChildren.push_back ( ( **lIt ).clone() );
    }

    for ( std::deque< Node* >::iterator lIt = mChildren.begin(); lIt != mChildren.end(); ++lIt )
    {
      mChildrenMap.insert ( std::make_pair ( ( **lIt ).mUid , *lIt ) );

      for ( boost::unordered_map< std::string , Node* >::iterator lSubMapIt = ( **lIt ).mChildrenMap.begin() ; lSubMapIt != ( **lIt ).mChildrenMap.end() ; ++lSubMapIt )
      {
        mChildrenMap.insert ( std::make_pair ( ( ( **lIt ).mUid ) +'.'+ ( lSubMapIt->first ) , lSubMapIt->second ) );
      }
    }

    return *this;
  }


  Node* Node::clone ( ) const
  {
    return new Node ( *this );
  }




  Node::~Node()
  {
    for ( std::deque< Node* >::iterator lIt = mChildren.begin(); lIt != mChildren.end(); ++lIt )
    {
      if ( *lIt )
      {
        delete ( *lIt );
        ( *lIt ) = NULL;
      }
    }

    mChildren.clear();
    mChildrenMap.clear();
  }


  bool Node::operator == ( const Node& aNode ) const
  {
    return this->getAddress() == aNode.getAddress() &&
           this->getMask() == aNode.getMask() &&
           this->getPermission() == aNode.getPermission() &&
           this->getId() == aNode.getId();
  }


  const std::string& Node::getId() const
  {
    return mUid;
  }

  const uint32_t& Node::getAddress() const
  {
    return mAddr;
  }

  const uint32_t& Node::getMask() const
  {
    return mMask;
  }

  const defs::BlockReadWriteMode& Node::getMode() const
  {
    return mMode;
  }

  const uint32_t& Node::getSize() const
  {
    return mSize;
  }

  const defs::NodePermission& Node::getPermission() const
  {
    return mPermission;
  }


  const std::string& Node::getTags() const
  {
    return mTags;
  }


  const std::string& Node::getDescription() const
  {
    return mDescription;
  }


  const std::string& Node::getModule() const
  {
    return mModule;
  }
  
  const std::vector< std::pair <std::string, std::string> >& Node::getParameters() const {
    return mParameters;
  }


  void Node::stream ( std::ostream& aStr , std::size_t aIndent ) const
  {
    aStr << std::setfill ( '0' ) << std::uppercase;
    aStr << '\n' << std::string ( aIndent , ' ' ) << "+ ";
    aStr << "Node \"" << mUid << "\", ";

    if ( &typeid ( *this ) != &typeid ( Node ) )
    {
      aStr << "of type \"";
#ifdef __GNUG__
      // this is fugly but necessary due to the way that typeid::name() returns the object type name under g++.
      int lStatus ( 0 );
      aStr << abi::__cxa_demangle ( typeid ( *this ).name() , 0 , 0 , &lStatus );
#else
      aStr << typeid ( *this ).name();
#endif
      aStr << "\", ";
    }

    switch ( mMode )
    {
      case defs::SINGLE:
        aStr << "SINGLE register, "
             << std::hex << "Address 0x" << std::setw ( 8 ) << mAddr << ", "
             << std::hex << "Mask 0x" << std::setw ( 8 ) << mMask << ", "
             << "Permissions " << ( mPermission&defs::READ?'r':'-' ) << ( mPermission&defs::WRITE?'w':'-' ) ;
        break;
      case defs::INCREMENTAL:
        aStr << "INCREMENTAL block, "
             << std::dec << "Size " << mSize << ", "
             << std::hex << "Addresses [0x" << std::setw ( 8 ) << mAddr << "-" << std::setw ( 8 ) << ( mAddr+mSize-1 ) << "], "
             << "Permissions " << ( mPermission&defs::READ?'r':'-' ) << ( mPermission&defs::WRITE?'w':'-' ) ;
        break;
      case defs::NON_INCREMENTAL:
        aStr << "NON-INCREMENTAL block, ";

        if ( mSize != 1 )
        {
          aStr << std::dec << "Size " << mSize << ", ";
        }

        aStr << std::hex << "Address 0x"  << std::setw ( 8 ) << mAddr << ", "
             << "Permissions " << ( mPermission&defs::READ?'r':'-' ) << ( mPermission&defs::WRITE?'w':'-' ) ;
        break;
      case defs::HIERARCHICAL:
        aStr << std::hex << "Address 0x" << std::setw ( 8 ) << mAddr;
        break;
    }

    if ( mTags.size() )
    {
      aStr << ", Tags \"" << mTags << "\"";
    }

    if ( mDescription.size() )
    {
      aStr << ", Description \"" << mDescription << "\"";
    }

    if ( mModule.size() )
    {
      aStr << ", Module \"" << mModule << "\"";
    }
    
    if ( mParameters.size() )
    {
      aStr << ", Parameters: ";
      for ( std::vector< std::pair< std::string, std::string> >::const_iterator lIt = mParameters.begin(); lIt != mParameters.end(); ++lIt )
      {
        aStr << lIt->first << "=" << lIt->second;
      }
    }
    
    for ( std::deque< Node* >::const_iterator lIt = mChildren.begin(); lIt != mChildren.end(); ++lIt )
    {
      ( **lIt ).stream ( aStr , aIndent+2 );
    }
  }


  Node& Node::getNode ( const std::string& aId )
  {
    if ( aId.size() == 0 )
    {
      return *this;
    }

    boost::unordered_map< std::string , Node* >::const_iterator lIt = mChildrenMap.find ( aId );

    if ( lIt==mChildrenMap.end() )
    {
      exception::NoBranchFoundWithGivenUID lExc;
      log ( lExc , "No branch found with ID-path " ,  Quote ( aId ) );
      std::size_t lPos ( std::string::npos );
      bool lPartialMatch ( false );

      while ( true )
      {
        lPos = aId.rfind ( '.' , lPos );

        if ( lPos == std::string::npos )
        {
          break;
        }

        boost::unordered_map< std::string , Node* >::const_iterator lIt = mChildrenMap.find ( aId.substr ( 0 , lPos ) );

        if ( lIt!=mChildrenMap.end() )
        {
          log ( lExc , "Partial match " ,  Quote ( aId.substr ( 0 , lPos ) ) , " found for ID-path " ,  Quote ( aId ) );
          log ( lExc , "Tree structure of partial match is:" , * ( lIt->second ) );
          lPartialMatch = true;
          break;
        }

        lPos--;
      }

      if ( !lPartialMatch )
      {
        log ( lExc , "Not even a partial match found for ID-path " ,  Quote ( aId ) , ". If this address looks correct, please check for leading, trailing and stray whitespace.\nTree structure is:" , *this );
      }

      throw lExc;
    }

    return * ( lIt->second );
  }


  std::vector<std::string> Node::getNodes() const
  {
    std::vector<std::string> lNodes;
    lNodes.reserve ( mChildrenMap.size() ); //prevent reallocations

    for ( boost::unordered_map< std::string , Node* >::const_iterator lIt = mChildrenMap.begin(); lIt != mChildrenMap.end(); ++lIt )
    {
      lNodes.push_back ( lIt->first );
    }

    return lNodes;
  }

  std::vector<std::string> Node::getNodes ( const std::string& aRegex ) const
  {
    std::vector<std::string> lNodes;
    lNodes.reserve ( mChildrenMap.size() ); //prevent reallocations
    log ( Info() , "Regular Expression : " , aRegex );

    for ( boost::unordered_map< std::string , Node* >::const_iterator lIt = mChildrenMap.begin(); lIt != mChildrenMap.end(); ++lIt )
    {
      boost::cmatch lMatch;

      if ( boost::regex_match ( lIt->first.c_str() , lMatch , boost::regex ( aRegex ) ) ) //to allow partial match, add  boost::match_default|boost::match_partial  as fourth argument
      {
        log ( Info() , lIt->first , " matches" );
        lNodes.push_back ( lIt->first );
      }
    }

    //bit dirty but since the hash map sorts them by the hash, not the value, they are completely scrambled here making it very hard to use.
    std::sort ( lNodes.begin(), lNodes.end() );
    return lNodes;
  }

  ValHeader  Node::write ( const uint32_t& aValue ) const
  {
    if ( mPermission & defs::WRITE )
    {
      if ( mMask == defs::NOMASK )
      {
        return mHw->getClient().write ( mAddr , aValue );
      }
      else
      {
        return mHw->getClient().write ( mAddr , aValue , mMask );
      }
    }
    else
    {
      exception::WriteAccessDenied lExc;
      log ( lExc , "Node " , Quote ( mUid ) , ": permissions denied write access" );
      throw lExc;
    }
  }


  ValHeader  Node::writeBlock ( const std::vector< uint32_t >& aValues ) const // , const defs::BlockReadWriteMode& aMode )
  {
    if ( ( mMode == defs::SINGLE ) && ( aValues.size() != 1 ) ) //We allow the user to call a bulk access of size=1 to a single register
    {
      exception::BulkTransferOnSingleRegister lExc;
      log ( lExc , "Bulk Transfer requested on single register node " , Quote ( mUid ) );
      log ( lExc , "If you were expecting an incremental write, please modify your address file to add the 'mode=",  Quote ( "incremental" ) , "' flags there" );
      throw lExc;
    }
    else
    {
      if ( ( mSize != 1 ) && ( aValues.size() >mSize ) )
      {
        exception::BulkTransferRequestedTooLarge lExc;
        log ( lExc , "Requested bulk write of greater size than the specified endpoint size of node ", Quote ( mUid ) );
        throw lExc;
      }

      if ( mPermission & defs::WRITE )
      {
        return mHw->getClient().writeBlock ( mAddr , aValues , mMode ); //aMode );
      }
      else
      {
        exception::WriteAccessDenied lExc;
        log ( lExc , "Node " , Quote ( mUid ) , ": permissions denied write access" );
        throw lExc;
      }
    }
  }


  ValWord< uint32_t > Node::read() const
  {
    if ( mPermission & defs::READ )
    {
      if ( mMask == defs::NOMASK )
      {
        return mHw->getClient().read ( mAddr );
      }
      else
      {
        return mHw->getClient().read ( mAddr , mMask );
      }
    }
    else
    {
      exception::ReadAccessDenied lExc;
      log ( lExc , "Node " , Quote ( mUid ) , ": permissions denied read access" );
      throw lExc;
    }
  }


  ValVector< uint32_t > Node::readBlock ( const uint32_t& aSize ) const //, const defs::BlockReadWriteMode& aMode )
  {
    if ( ( mMode == defs::SINGLE ) && ( aSize != 1 ) ) //We allow the user to call a bulk access of size=1 to a single register
    {
      exception::BulkTransferOnSingleRegister lExc;
      log ( lExc , "Bulk Transfer requested on single register node ", Quote ( mUid ) );
      log ( lExc , "If you were expecting an incremental read, please modify your address file to add the 'mode=",  Quote ( "incremental" ) , "' flags there" );
      throw lExc;
    }
    else
    {
      if ( ( mSize != 1 ) && ( aSize>mSize ) )
      {
        exception::BulkTransferRequestedTooLarge lExc;
        log ( lExc , "Requested bulk read of greater size than the specified endpoint size of node " , Quote ( mUid ) );
        throw lExc;
      }

      if ( mPermission & defs::READ )
      {
        return mHw->getClient().readBlock ( mAddr , aSize , mMode ); //aMode );
      }
      else
      {
        exception::ReadAccessDenied lExc;
        log ( lExc , "Node " , Quote ( mUid ) , ": permissions denied read access" );
        throw lExc;
      }
    }
  }

  // ValWord< int32_t > Node::readSigned()
  // {
  // try
  // {
  // if ( mPermission & defs::READ )
  // {
  // if ( mMask == defs::NOMASK )
  // {
  // return mHw->getClient().readSigned ( mAddr );
  // }
  // else
  // {
  // return mHw->getClient().readSigned ( mAddr , mMask );
  // }
  // }
  // else
  // {
  // log ( Error() , "Node permissions denied read access" );
  // throw exception::// ReadAccessDenied();
  // }
  // }
  // catch ( uhal::exception& aExc )
  // {
  // aExc.throw r;
  // }
  // catch ( const std::exception& aExc )
  // {
  // throw // StdException ( aExc );
  // }
  // }


  // ValVector< int32_t > Node::readBlockSigned ( const uint32_t& aSize ) //, const defs::BlockReadWriteMode& aMode )
  // {
  // try
  // {
  // if ( ( mMode == defs::SINGLE ) && ( aSize != 1 ) ) //We allow the user to call a bulk access of size=1 to a single register
  // {
  // log ( Error() , "Bulk Transfer requested on single register node" );
  // log ( Error() , "If you were expecting an incremental read, please modify your address file to add the 'mode=",  Quote ( "incremental" ) , "' flags there" );
  // throw exception::// BulkTransferOnSingleRegister();
  // }
  // else
  // {
  // if ( ( mSize != 1 ) && ( aSize>mSize ) )
  // {
  // log ( Error() , "Requested bulk read of greater size than the specified endpoint size" );
  // throw exception::// BulkTransferRequestedTooLarge();
  // }

  // if ( mPermission & defs::READ )
  // {
  // return mHw->getClient().readBlockSigned ( mAddr , aSize , mMode ); //aMode );
  // }
  // else
  // {
  // log ( Error() , "Node permissions denied read access" );
  // throw exception::// ReadAccessDenied();
  // }
  // }
  // }
  // catch ( uhal::exception& aExc )
  // {
  // aExc.throw r;
  // }
  // catch ( const std::exception& aExc )
  // {
  // throw // StdException ( aExc );
  // }
  // }




  // ValWord< uint32_t > Node::rmw_bits ( const uint32_t& aANDterm , const uint32_t& aORterm )
  // {
  // try
  // {
  // if ( mPermission == defs::READWRITE )
  // {
  // return mHw->getClient().rmw_bits ( mAddr , aANDterm , aORterm );
  // }
  // else
  // {
  // log ( Error() , "Node permissions denied read/write access" );
  // throw exception::// ReadAccessDenied();
  // }
  // }
  // catch ( uhal::exception& aExc )
  // {
  // aExc.throw r;
  // }
  // catch ( const std::exception& aExc )
  // {
  // throw // StdException ( aExc );
  // }
  // }



  // ValWord< uint32_t > Node::rmw_sum ( const int32_t& aAddend )
  // {
  // try
  // {
  // if ( mPermission == defs::READWRITE )
  // {
  // return mHw->getClient().rmw_sum ( mAddr , aAddend );
  // }
  // else
  // {
  // log ( Error() , "Node permissions denied read/write access" );
  // throw exception::// ReadAccessDenied();
  // }
  // }
  // catch ( uhal::exception& aExc )
  // {
  // aExc.throw r;
  // }
  // catch ( const std::exception& aExc )
  // {
  // throw // StdException ( aExc );
  // }
  // }

  ClientInterface& Node::getClient() const
  {
    return mHw->getClient();
  }

}
