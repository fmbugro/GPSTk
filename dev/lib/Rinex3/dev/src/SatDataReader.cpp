#pragma ident "$Id: SatDataReader.cpp 1161 2008-03-27 17:16:22Z ckiesch $"

/**
 * @file SatDataReader.cpp
 * File stream for satellite file data in PRN_GPS-like format.
 */

//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 2.1 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007
//
//============================================================================




#include "SatDataReader.hpp"


namespace gpstk
{

      // Method to store satellite data in this class' data map
   void SatDataReader::loadData(void)
      throw(FFStreamError, gpstk::StringUtils::StringException)
   {

         // Do this until end-of-file reached or something else happens
      while(1)
      {
         try
         {
            std::string line;

            formattedGetLine(line, true);

               // If line is too long, we throw an exception
            if (line.size()>255)
            {
               FFStreamError e("Line too long");
               GPSTK_THROW(e);
            }

               // Let's find and strip comments, wherever they are
            if( StringUtils::firstWord(line)[0] == '#' )
            {
               formattedGetLine(line, true);
            }

            std::string::size_type idx = line.find('#');
            if( !(idx == std::string::npos) )
            {
               line = line.substr(0, idx);
            }

               // We erase the header (first line)
            if( StringUtils::firstWord(line) == "Launch" )
            {
               formattedGetLine(line, true);
            }

               // Remove trailing and leading blanks
            line = StringUtils::strip(line);

               // Skip blank lines
            if (line.size()==0)
            {
               continue;
            }

               // Let's start to get data out of file
               // Launch date
            string ldate(StringUtils::stripFirstWord(line));
               // Deactivation date
            string ddate(StringUtils::stripFirstWord(line));
               // GPS number
            string gnumber(StringUtils::stripFirstWord(line));
               // PRN number
            string prn(StringUtils::stripFirstWord(line));
               // Block tipe
            string block(StringUtils::upperCase(
               StringUtils::stripFirstWord(line)));

               // Get satellite id. If it doesn't fit GPS or Glonass, it is 
               // marked as unknown
            SatID sat(StringUtils::asInt(prn),SatID::systemUnknown);
               // Let's identify satellite system
            if(block[0] == 'I')
            {
               sat.system = SatID::systemGPS;
            }
            else
            { 
               if (block.substr(0, 3) == "GLO")
               {
                  sat.system = SatID::systemGlonass;
               }
            }

               // Declare the structure to store data
            SatDataReader::svData data;

            data.block = block;
            data.gpsNumber = StringUtils::asInt(gnumber);

               // Get launch date in a proper format
            if(ldate[0] != '0')
            {
                ldate = StringUtils::translate(ldate, "-", " ");
                data.launchDate.setToString(ldate, "%Y %m %d");
            }

               // Get deactivation date in a proper format
            if(ddate[0] != '0')
            {
                ddate = StringUtils::translate(ddate, "-", " ");
                data.deactivationDate.setToString(ddate, "%Y %m %d");
            }


               // Insert data in data map
            setData(sat, data);

         }  // End of try block
         catch (EndOfFile& e)
         {
            return;
         }
         catch (...)
         {
            return;
         }

      } // End of while(1)

   } // End of SatDataReader::loadData()



      // Method to open AND load satellite data file.
   void SatDataReader::open(const char* fn)
   {
      FFTextStream::open(fn, std::ios::in);
      loadData();

      return;
   }


      // Method to open AND load satellite data file.
   void SatDataReader::open(const string& fn)
   {
      FFTextStream::open(fn.c_str(), std::ios::in);
      loadData();
            
      return;
   }



      /* Method to get the block type of a given SV at a given epoch.
       *
       * @param sat   Satellite ID.
       * @param epoch Epoch of interest.
       *
       * @return String containing satellite's block. If satellite is 
       * not found or epoch is out of proper launch/deactivation bounds, 
       * this method will return an empty string.
       */
   string SatDataReader::getBlock(const SatID& sat,
                                  const DayTime& epoch) const
   {

         // Create a pair of range belonging to this SatID
      pair<satDataIt, satDataIt> range = SatelliteData.equal_range(sat);

         // If SatID is not found, an empty string is returned
      if(range.first == range.second)
      {
         return "";
      }

         // Declare an iterator to travel in this range
      satDataIt iter(range.first);

         // If this epoch is before launch date, return an empty string
      if( (*iter).second.launchDate > epoch )
      {
         return "";
      }

         // Increment iterator "iter" if we are not yet at proper epoch range
      while( (*iter).second.deactivationDate < epoch )
      {
         ++iter;
      }

         // Test if epoch is after corresponding launch date
      if( (*iter).second.launchDate > epoch )
      {
         return "";
      }
        

      return ((*iter).second.block);

   } // End of SatDataReader::getBlock()


      /* Method to get the GPS number of a given SV at a given epoch.
       *
       * @param sat   Satellite ID.
       * @param epoch Epoch of interest.
       *
       * @return Integer containing satellite's block. If satellite is 
       * not found or epoch is out of proper launch/deactivation bounds, 
       * this method will return -1.
       */
   int SatDataReader::getGPSNumber(const SatID& sat,
                                   const DayTime& epoch) const 
   {

         // Create a pair of range belonging to this SatID
      pair<satDataIt, satDataIt> range = SatelliteData.equal_range(sat);

         // If SatID is not found, -1 is returned
      if(range.first == range.second)
      {
         return -1;
      }

         // Declare an iterator to travel in this range
      satDataIt iter(range.first);

         // If this epoch is before launch date, return -1
      if( (*iter).second.launchDate > epoch )
      {
         return -1;
      }

         // Increment iterator "iter" if we are not yet at proper epoch range
      while( (*iter).second.deactivationDate < epoch )
      {
         ++iter;
      }

         // Test if epoch is after corresponding launch date
      if( (*iter).second.launchDate > epoch )
      {
         return -1;
      }
        

      return ((*iter).second.gpsNumber);

   } // End of SatDataReader::getGPSNumber()


      /* Method to get the launch date of a given SV.
       *
       * @param sat   Satellite ID.
       * @param epoch Epoch of interest.
       *
       * @return DayTime object containing satellite's launch date. If 
       * satellite is not found or epoch is out of proper launch/deactivation 
       * bounds, this method will return DayTime::END_OF_TIME.
       */
   DayTime SatDataReader::getLaunchDate(const SatID& sat,
                                        const DayTime& epoch) const 
   {

         // Create a pair of range belonging to this SatID
      pair<satDataIt, satDataIt> range = SatelliteData.equal_range(sat);

         // If SatID is not found, DayTime::END_OF_TIME is returned
      if(range.first == range.second)
      {
         return DayTime::END_OF_TIME;
      }

         // Declare an iterator to travel in this range
      satDataIt iter(range.first);

         // If this epoch is before launch date, return DayTime::END_OF_TIME
      if( (*iter).second.launchDate > epoch )
      {
         return DayTime::END_OF_TIME;
      }

         // Increment iterator "iter" if we are not yet at proper epoch range
      while( (*iter).second.deactivationDate < epoch )
      {
         ++iter;
      }

         // Test if epoch is after corresponding launch date
      if( (*iter).second.launchDate > epoch )
      {
         return DayTime::END_OF_TIME;
      }

      return ((*iter).second.launchDate);

   } // End of SatDataReader::getLaunchDate()


      /* Method to get the deactivation date of a given SV.
       *
       * @param sat   Satellite ID.
       * @param epoch Epoch of interest.
       *
       * @return DayTime object containing satellite's deactivation date. If 
       * satellite is not found, epoch is out of proper launch/deactivation 
       * bounds or satellite is still active, this method will return 
       * DayTime::BEGINNING_OF_TIME.
       */
   DayTime SatDataReader::getDeactivationDate(const SatID& sat,
                                              const DayTime& epoch) const 
   {

         // Create a pair of range belonging to this SatID
      pair<satDataIt, satDataIt> range = SatelliteData.equal_range(sat);

         // If SatID is not found, DayTime::BEGINNING_OF_TIME is returned
      if(range.first == range.second)
      {
         return DayTime::BEGINNING_OF_TIME;
      }

         // Declare an iterator to travel in this range
      satDataIt iter(range.first);

         // If this epoch is before launch date, return BEGINNING_OF_TIME
      if( (*iter).second.launchDate > epoch )
      {
         return DayTime::BEGINNING_OF_TIME;
      }

         // Increment iterator "iter" if we are not yet at proper epoch range
      while( (*iter).second.deactivationDate < epoch )
      {
         ++iter;
      }

         // Test if epoch is after corresponding launch date
      if( (*iter).second.launchDate > epoch )
      {
         return DayTime::BEGINNING_OF_TIME;
      }

      return ((*iter).second.deactivationDate);

   } // End of SatDataReader::getDeactivationDate()


} // namespace