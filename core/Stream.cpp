/*
 Stream.cpp - adds parsing methods to Stream class
 Copyright (c) 2008 David A. Mellis.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Created July 2011
 parsing functions based on TextFinder library by Michael Margolis
 */

#include "Arduino.h"
#include "Stream.h"

#define PARSE_TIMEOUT 5  // default number of seconds to wait
#define NO_SKIP_CHAR  1  // a magic char not found in a valid ASCII numeric field

// private method to read stream with timeout
int Stream::timedRead()
{
  //Serial.println(_timeout);
  this->_startMillis = millis();
  while(millis() - this->_startMillis < (this->_timeout * 1000L))
  {
    if (this->available() > 0) {
      return this->read();
    }
  }
  return -1;     // -1 indicates timeout
}

// returns the next digit in the stream or -1 if timeout
// discards non-numeric characters
int Stream::getNextDigit()
{
  int c;
  do{
    c = timedRead();
    if( c < 0)
      return c;  // timeout
  }
  while( c != '-' && (c < '0' || c > '9') ) ;

return c;
}

// Public Methods
//////////////////////////////////////////////////////////////

void Stream::setTimeout( long timeout)  // sets the maximum number of seconds to wait
{
   this->_timeout = timeout;
}

 // find returns true if the target string is found
bool  Stream::find(char *target)
{
  return findUntil(target, NULL);
}

// reads data from the stream until the target string of given length is found
// returns true if target string is found, false if timed out
bool Stream::find(char *target, size_t length)
{
  return findUntil(target, length, NULL, 0);
}

// as find but search ends if the terminator string is found
bool  Stream::findUntil(char *target, char *terminator)
{
  return findUntil(target, strlen(target), terminator, strlen(terminator));
}

// reads data from the stream until the target string of the given length is found
// search terminated if the terminator string is found
// returns true if target string is found, false if terminated or timed out
bool Stream::findUntil(char *target, size_t targetLen, char *terminator, size_t termLen)
{
  size_t index = 0;  // maximum target string length is 64k bytes!
  size_t termIndex = 0;
  int c;

  if( *target == 0)
     return true;   // return true if target is a null string
  while( (c = timedRead()) > 0){
    if( c == target[index]){
    //////Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
      if(++index >= targetLen){ // return true if all chars in the target match
        return true;
      }
    }
    else{
      index = 0;  // reset index if any char does not match
    }
    if(termLen > 0 && c == terminator[termIndex]){
       if(++termIndex >= termLen)
         return false;       // return false if terminate string found before target string
    }
    else
        termIndex = 0;
  }
  return false;
}


// returns the first valid (long) integer value from the current position.
// initial characters that are not digits (or the minus sign) are skipped
// function is terminated by the first character that is not a digit.
long Stream::parseInt()
{
  return parseInt(NO_SKIP_CHAR); // terminate on first non-digit character (or timeout)
}

// as above but a given skipChar is ignored
// this allows format characters (typically commas) in values to be ignored
long Stream::parseInt(char skipChar)
{
  boolean isNegative = false;
  long value = 0;
  int c;

  c = getNextDigit();
  // ignore non numeric leading characters
  if(c < 0)
    return 0; // zero returned if timeout

  do{
    if(c == skipChar)
      ; // ignore this charactor
    else if(c == '-')
      isNegative = true;
    else if(c >= '0' && c <= '9')        // is c a digit?
      value = value * 10 + c - '0';
    c = timedRead();
  }
  while( (c >= '0' && c <= '9')  || c == skipChar );

  if(isNegative)
    value = -value;
  return value;
}


// as parseInt but returns a floating point value
float Stream::parseFloat()
{
  parseFloat(NO_SKIP_CHAR);
}

// as above but the given skipChar is ignored
// this allows format characters (typically commas) in values to be ignored
float Stream::parseFloat(char skipChar){
  boolean isNegative = false;
  boolean isFraction = false;
  long value = 0;
  float fValue;
  char c;
  float fraction = 1.0;

  c = getNextDigit();
    // ignore non numeric leading characters
  if(c < 0)
    return 0; // zero returned if timeout

  do{
    if(c == skipChar)
      ; // ignore
    else if(c == '-')
      isNegative = true;
    else if (c == '.')
      isFraction = true;
    else if(c >= '0' && c <= '9')  {      // is c a digit?
      value = value * 10 + c - '0';
      if(isFraction)
         fraction *= 0.1;
    }
    c = timedRead();
  }
  while( (c >= '0' && c <= '9')  || c == '.' || c == skipChar );

  if(isNegative)
    value = -value;
  if(isFraction)
    return value * fraction;
  else
    return value;
}

// read characters from stream into buffer
// terminates if length characters have been read, null is detected or timeout (see setTimeout)
// returns the number of characters placed in the buffer (0 means no valid data found)
int Stream::readChars( char *buffer, size_t length)
{
   return readCharsUntil( 0, buffer, length);
}


// as readChars with terminator character
// terminates if length characters have been read, timeout, or if the terminator character  detected
// returns the number of characters placed in the buffer (0 means no valid data found)

int Stream::readCharsUntil( char terminator, char *buffer, size_t length)
{
 int index = 0;
    *buffer = 0;
    while(index < length ){
      int c = timedRead();
      if( c <= 0 ){
        return 0;   // timeout returns 0 !
      }
      else if( c == terminator){
        buffer[index] = 0; // terminate the string
        return index;               // data got successfully
      }
      else{
        buffer[index++] = (char)c;
      }
    }
    buffer[index] = 0;
    return index; // here if buffer is full before detecting the terminator
}


// read characters found between pre_string and terminator into a buffer
// terminated when the terminator character is matched or the buffer is full
// returns the number of bytes placed in the buffer (0 means no valid data found)
int Stream::readCharsBetween( char *pre_string, char terminator, char *buffer, size_t length)
{
  if( find( pre_string) ){
    return readCharsUntil(terminator, buffer, length);
  }
  return 0;    //failed to find the prestring
}
