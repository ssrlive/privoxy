@REM /*********************************************************************  
@REM  *                                                                      
@REM  * File        :  $Source$                                              
@REM  *                                                                      
@REM  * Purpose     :  Start the Privoxy statistics viewer on OS/2 platforms
@REM  *                                                                      
@REM  * Copyright   :  Written by and Copyright (C) 2003 the SourceForge     
@REM  *                Privoxy team. http://www.privoxy.org/                 
@REM  *                                                                      
@REM  *                Based on the Internet Junkbuster originally written   
@REM  *                by and Copyright (C) 1997 Anonymous Coders and        
@REM  *                Junkbusters Corporation.  http://www.junkbusters.com  
@REM  *                                                                      
@REM  *                This program is free software; you can redistribute it
@REM  *                and/or modify it under the terms of the GNU General   
@REM  *                Public License as published by the Free Software      
@REM  *                Foundation; either version 2 of the License, or (at   
@REM  *                your option) any later version.                       
@REM  *                                                                      
@REM  *                This program is distributed in the hope that it will  
@REM  *                be useful, but WITHOUT ANY WARRANTY; without even the 
@REM  *                implied warranty of MERCHANTABILITY or FITNESS FOR A  
@REM  *                PARTICULAR PURPOSE.  See the GNU General Public       
@REM  *                License for more details.                             
@REM  *                                                                      
@REM  *                The GNU General Public License should be included with
@REM  *                this file.  If not, you can view it at                
@REM  *                http://www.gnu.org/copyleft/gpl.html                  
@REM  *                or write to the Free Software Foundation, Inc., 59    
@REM  *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
@REM  *                                                                      
@REM  *********************************************************************/ 
@REM
@REM Syntax:
@REM
@REM acon [port_to_serve]
@REM
@REM - Requires Swing (i.e. swingall.jar) in CLASSPATH or Java 1.3+
@REM
@start /MIN /C java -classpath %CLASSPATH%;ActivityConsole.jar org.privoxy.activityconsole.ActivityConsole %1
