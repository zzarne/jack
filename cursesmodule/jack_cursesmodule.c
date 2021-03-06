/*
 *   This is a curses implementation for Python.
 *
 *   Based on a prior work by Lance Ellinghaus
 *   (version 1.2 of this module
 *    Copyright 1994 by Lance Ellinghouse,
 *    Cathedral City, California Republic, United States of America.)
 *   Updated, fixed and heavily extended by Oliver Andrich
 *
 *   Copyright 1996,1997 by Oliver Andrich,
 *   Koblenz, Germany
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this source file to use, copy, modify, merge, or publish it
 *   subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included
 *   in all copies or in any new file that contains a substantial portion of
 *   this file.
 *
 *   THE  AUTHOR  MAKES  NO  REPRESENTATIONS ABOUT  THE  SUITABILITY  OF
 *   THE  SOFTWARE FOR  ANY  PURPOSE.  IT IS  PROVIDED  "AS IS"  WITHOUT
 *   EXPRESS OR  IMPLIED WARRANTY.  THE AUTHOR DISCLAIMS  ALL WARRANTIES
 *   WITH  REGARD TO  THIS  SOFTWARE, INCLUDING  ALL IMPLIED  WARRANTIES
 *   OF   MERCHANTABILITY,  FITNESS   FOR  A   PARTICULAR  PURPOSE   AND
 *   NON-INFRINGEMENT  OF THIRD  PARTY  RIGHTS. IN  NO  EVENT SHALL  THE
 *   AUTHOR  BE LIABLE  TO  YOU  OR ANY  OTHER  PARTY  FOR ANY  SPECIAL,
 *   INDIRECT,  OR  CONSEQUENTIAL  DAMAGES  OR  ANY  DAMAGES  WHATSOEVER
 *   WHETHER IN AN  ACTION OF CONTRACT, NEGLIGENCE,  STRICT LIABILITY OR
 *   ANY OTHER  ACTION ARISING OUT OF  OR IN CONNECTION WITH  THE USE OR
 *   PERFORMANCE OF THIS SOFTWARE.
 */

/* CVS: $Id$ */

/* Release Number */

char *PyCursesVersion = "1.5b1";

/* Includes */

#include "Python.h"
#include <curses.h>

#ifdef __sgi__
 /* No attr_t type is available */
typedef chtype attr_t;
#endif

/* Definition of exception curses.error */

static PyObject *PyCursesError;

/* general error messages */
static char *catchall_ERR  = "curses function returned ERR";
static char *catchall_NULL = "curses function returned NULL";

/* Tells whether initscr() has been called to initialise curses.  */
static int initialised = FALSE;

/* Tells whether start_color() has been called to initialise colorusage. */
static int initialisedcolors = FALSE;

/* Utility Macros */
#define ARG_COUNT(X) \
	(((X) == NULL) ? 0 : (PyTuple_Check(X) ? PyTuple_Size(X) : 1))

#define PyCursesInitialised \
  if (initialised != TRUE) { \
                  PyErr_SetString(PyCursesError, \
                                  "must call initscr() first"); \
                  return NULL; }

#define PyCursesInitialisedColor \
  if (initialisedcolors != TRUE) { \
                  PyErr_SetString(PyCursesError, \
                                  "must call start_color() first"); \
                  return NULL; }

/* Utility Functions */

/*
 * Check the return code from a curses function and return None 
 * or raise an exception as appropriate.
 */

static PyObject *
PyCursesCheckERR(code, fname)
     int code;
     char *fname;
{
  char buf[100];

  if (code != ERR) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    if (fname == NULL) {
      PyErr_SetString(PyCursesError, catchall_ERR);
    } else {
      strcpy(buf, fname);
      strcat(buf, "() returned ERR");
      PyErr_SetString(PyCursesError, buf);
    }
    return NULL;
  }
}

int 
PyCurses_ConvertToChtype(obj, ch)
        PyObject *obj;
        chtype *ch;
{
  if (PyInt_Check(obj)) {
    *ch = (chtype) PyInt_AsLong(obj);
  } else if(PyString_Check(obj) &
	    (PyString_Size(obj) == 1)) {
    *ch = (chtype) *PyString_AsString(obj);
  } else {
    return 0;
  }
  return 1;
}

/*****************************************************************************
 The Window Object
******************************************************************************/

/* Definition of the window object and window type */

typedef struct {
	PyObject_HEAD
	WINDOW *win;
} PyCursesWindowObject;

PyTypeObject PyCursesWindow_Type;

#define PyCursesWindow_Check(v)	 ((v)->ob_type == &PyCursesWindow_Type)

/* Function Prototype Macros - They are ugly but very, very useful. ;-)

   X - function name
   TYPE - parameter Type
   ERGSTR - format string for construction of the return value
   PARSESTR - format string for argument parsing
   */

#define Window_NoArgNoReturnFunction(X) \
static PyObject *PyCursesWindow_ ## X (self, arg) \
     PyCursesWindowObject * self; PyObject * arg; \
{ if (!PyArg_NoArgs(arg)) return NULL; \
  return PyCursesCheckERR(X(self->win), # X); }

#define Window_NoArgTrueFalseFunction(X) \
static PyObject * PyCursesWindow_ ## X (self,arg) \
     PyCursesWindowObject * self; PyObject * arg; \
{ \
  if (!PyArg_NoArgs(arg)) return NULL; \
  if (X (self->win) == FALSE) { Py_INCREF(Py_False); return Py_False; } \
  else { Py_INCREF(Py_True); return Py_True; } }

#define Window_NoArgNoReturnVoidFunction(X) \
static PyObject * PyCursesWindow_ ## X (self,arg) \
     PyCursesWindowObject * self; \
     PyObject * arg; \
{ \
  if (!PyArg_NoArgs(arg)) return NULL; \
  X(self->win); Py_INCREF(Py_None); return Py_None; }

#define Window_NoArg2TupleReturnFunction(X, TYPE, ERGSTR) \
static PyObject * PyCursesWindow_ ## X (self, arg) \
     PyCursesWindowObject *self; \
     PyObject * arg; \
{ \
  TYPE arg1, arg2; \
  if (!PyArg_NoArgs(arg)) return NULL; \
  X(self->win,arg1,arg2); return Py_BuildValue(ERGSTR, arg1, arg2); } 

#define Window_OneArgNoReturnVoidFunction(X, TYPE, PARSESTR) \
static PyObject * PyCursesWindow_ ## X (self, arg) \
     PyCursesWindowObject *self; \
     PyObject * arg; \
{ \
  TYPE arg1; \
  if (!PyArg_Parse(arg, PARSESTR, &arg1)) return NULL; \
  X(self->win,arg1); Py_INCREF(Py_None); return Py_None; }

#define Window_OneArgNoReturnFunction(X, TYPE, PARSESTR) \
static PyObject * PyCursesWindow_ ## X (self, arg) \
     PyCursesWindowObject *self; \
     PyObject * arg; \
{ \
  TYPE arg1; \
  if (!PyArg_Parse(arg,PARSESTR, &arg1)) return NULL; \
  return PyCursesCheckERR(X(self->win, arg1), # X); }

#define Window_TwoArgNoReturnFunction(X, TYPE, PARSESTR) \
static PyObject * PyCursesWindow_ ## X (self, arg) \
     PyCursesWindowObject *self; \
     PyObject * arg; \
{ \
  TYPE arg1, arg2; \
  if (!PyArg_Parse(arg,PARSESTR, &arg1, &arg2)) return NULL; \
  return PyCursesCheckERR(X(self->win, arg1, arg2), # X); }

/* ------------- WINDOW routines --------------- */

Window_NoArgNoReturnFunction(untouchwin)
Window_NoArgNoReturnFunction(touchwin)
Window_NoArgNoReturnFunction(redrawwin)
Window_NoArgNoReturnFunction(winsertln)
Window_NoArgNoReturnFunction(werase)
Window_NoArgNoReturnFunction(wdeleteln)

Window_NoArgTrueFalseFunction(is_wintouched)

Window_NoArgNoReturnVoidFunction(wsyncup)
Window_NoArgNoReturnVoidFunction(wsyncdown)
Window_NoArgNoReturnVoidFunction(wstandend)
Window_NoArgNoReturnVoidFunction(wstandout)
Window_NoArgNoReturnVoidFunction(wcursyncup)
Window_NoArgNoReturnVoidFunction(wclrtoeol)
Window_NoArgNoReturnVoidFunction(wclrtobot)
Window_NoArgNoReturnVoidFunction(wclear)

Window_OneArgNoReturnVoidFunction(idcok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnVoidFunction(immedok, int, "i;True(1) or False(0)")

Window_NoArg2TupleReturnFunction(getyx, int, "(ii)")
Window_NoArg2TupleReturnFunction(getbegyx, int, "(ii)")
Window_NoArg2TupleReturnFunction(getmaxyx, int, "(ii)")
Window_NoArg2TupleReturnFunction(getparyx, int, "(ii)")

Window_OneArgNoReturnFunction(wattron, attr_t, "l;attr")
Window_OneArgNoReturnFunction(wattroff, attr_t, "l;attr")
Window_OneArgNoReturnFunction(wattrset, attr_t, "l;attr")
Window_OneArgNoReturnFunction(clearok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(idlok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(keypad, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(leaveok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(nodelay, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(notimeout, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(scrollok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(winsdelln, int, "i;cnt")
Window_OneArgNoReturnFunction(syncok, int, "i;True(1) or False(0)")

Window_TwoArgNoReturnFunction(mvwin, int, "(ii);y,x")
Window_TwoArgNoReturnFunction(mvderwin, int, "(ii);y,x")
Window_TwoArgNoReturnFunction(wmove, int, "(ii);y,x")
#ifndef __sgi__
Window_TwoArgNoReturnFunction(wresize, int, "(ii);lines,columns")
#endif

/* Allocation and Deallocation of Window Objects */

static PyObject *
PyCursesWindow_New(win)
	WINDOW *win;
{
	PyCursesWindowObject *wo;

	wo = PyObject_New(PyCursesWindowObject, &PyCursesWindow_Type);
	if (wo == NULL) return NULL;
	wo->win = win;
	return (PyObject *)wo;
}

static void
PyCursesWindow_Dealloc(wo)
	PyCursesWindowObject *wo;
{
  if (wo->win != stdscr) delwin(wo->win);
  PyObject_Del(wo);
}

/* Addch, Addstr, Addnstr */

static PyObject *
PyCursesWindow_AddCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn, x, y, use_xy = FALSE;
  PyObject *temp;
  chtype ch = 0;
  attr_t attr = A_NORMAL;
  
  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg, "O;ch or int", &temp))
	  return NULL;
    break;
  case 2:
    if (!PyArg_Parse(arg, "(Ol);ch or int,attr", &temp, &attr))
      return NULL;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(iiO);y,x,ch or int", &y, &x, &temp))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iiOl);y,x,ch or int, attr", 
		     &y, &x, &temp, &attr))
      return NULL;
    use_xy = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addch requires 1 or 4 arguments");
    return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 or 3 must be a ch or an int");
    return NULL;
  }
  
  if (use_xy == TRUE)
    rtn = mvwaddch(self->win,y,x, ch | attr);
  else {
    rtn = waddch(self->win, ch | attr);
  }
  return PyCursesCheckERR(rtn, "addch");
}

static PyObject *
PyCursesWindow_AddStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"s;str", &str))
      return NULL;
    break;
  case 2:
    if (!PyArg_Parse(arg,"(sl);str,attr", &str, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(iis);int,int,str", &y, &x, &str))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iisl);int,int,str,attr", &y, &x, &str, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addstr requires 1 to 4 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwaddstr(self->win,y,x,str);
  else
    rtn = waddstr(self->win,str);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "addstr");
}

static PyObject *
PyCursesWindow_AddNStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn, x, y, n;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(si);str,n", &str, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(sil);str,n,attr", &str, &n, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iisi);y,x,str,n", &y, &x, &str, &n))
      return NULL;
    use_xy = TRUE;
    break;
  case 5:
    if (!PyArg_Parse(arg,"(iisil);y,x,str,n,attr", &y, &x, &str, &n, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addnstr requires 2 to 5 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwaddnstr(self->win,y,x,str,n);
  else
    rtn = waddnstr(self->win,str,n);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "addnstr");
}

static PyObject *
PyCursesWindow_Bkgd(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  PyObject *temp;
  chtype bkgd;
  attr_t attr = A_NORMAL;

  switch (ARG_COUNT(arg)) {
    case 1:
      if (!PyArg_Parse(arg, "O;ch or int", &temp))
        return NULL;
      break;
    case 2:
      if (!PyArg_Parse(arg,"(Ol);ch or int,attr", &temp, &attr))
        return NULL;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "bkgd requires 1 or 2 arguments");
      return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &bkgd)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 or 3 must be a ch or an int");
    return NULL;
  }

  return PyCursesCheckERR(wbkgd(self->win, bkgd | A_NORMAL), "bkgd");
}

static PyObject *
PyCursesWindow_BkgdSet(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  PyObject *temp;
  chtype bkgd;
  attr_t attr = A_NORMAL;

  switch (ARG_COUNT(arg)) {
    case 1:
      if (!PyArg_Parse(arg, "O;ch or int", &temp))
        return NULL;
      break;
    case 2:
      if (!PyArg_Parse(arg,"(Ol);ch or int,attr", &temp, &attr))
        return NULL;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "bkgdset requires 1 or 2 arguments");
      return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &bkgd)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 or 3 must be a ch or an int");
    return NULL;
  }

  wbkgdset(self->win, bkgd | attr);
  return PyCursesCheckERR(0, "bkgdset");
}

static PyObject *
PyCursesWindow_Border(self, args)
     PyCursesWindowObject *self;
     PyObject *args;
{
  chtype ls, rs, ts, bs, tl, tr, bl, br;
  ls = rs = ts = bs = tl = tr = bl = br = 0;
  if (!PyArg_Parse(args,"|llllllll;ls,rs,ts,bs,tl,tr,bl,br",
                        &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br))
    return NULL;
  wborder(self->win, ls, rs, ts, bs, tl, tr, bl, br);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_Box(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  chtype ch1=0,ch2=0;
  if (!PyArg_NoArgs(arg)) {
    PyErr_Clear();
    if (!PyArg_Parse(arg,"(ll);vertint,horint", &ch1, &ch2))
      return NULL;
  }
  box(self->win,ch1,ch2);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_DelCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = wdelch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x", &y, &x))
      return NULL;
    rtn = mvwdelch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "delch requires 0 or 2 arguments");
    return NULL;
  }
  return PyCursesCheckERR(rtn, "[mv]wdelch");
}

static PyObject *
PyCursesWindow_DerWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;

  nlines = 0;
  ncols  = 0;
  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(ii);begin_y,begin_x",&begin_y,&begin_x))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(arg, "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "derwin requires 2 or 4 arguments");
    return NULL;
  }

  win = derwin(self->win,nlines,ncols,begin_y,begin_x);

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCursesWindow_EchoChar(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  PyObject *temp;
  chtype ch;
  attr_t attr = A_NORMAL;

  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"O;ch or int", &temp))
      return NULL;
    break;
  case 2:
    if (!PyArg_Parse(arg,"(Ol);ch or int,attr", &temp, &attr))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "echochar requires 1 or 2 arguments");


    return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 must be a ch or an int");
    return NULL;
  }
  
  if (self->win->_flags & _ISPAD)
    return PyCursesCheckERR(pechochar(self->win, ch | attr), 
			    "echochar");
  else
    return PyCursesCheckERR(wechochar(self->win, ch | attr), 
			    "echochar");
}

static PyObject *
PyCursesWindow_GetBkgd(self, arg)
     PyCursesWindowObject *self;
     PyObject *arg;
{
  if (!PyArg_NoArgs(arg))
    return NULL;
  return PyInt_FromLong((long) getbkgd(self->win));
}

static PyObject *
PyCursesWindow_GetCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  chtype rtn;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = wgetch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn = mvwgetch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getch requires 0 or 2 arguments");
    return NULL;
  }
  return PyInt_FromLong(rtn);
}

static PyObject *
PyCursesWindow_GetKey(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  chtype rtn;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = wgetch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn = mvwgetch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getch requires 0 or 2 arguments");
    return NULL;
  }
  if (rtn<=255)
    return Py_BuildValue("c", rtn);
  else
    return PyString_FromString((char *)keyname(rtn));
}

static PyObject *
PyCursesWindow_GetStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y, n;
  char rtn[1024]; /* This should be big enough.. I hope */
  int rtn2;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn2 = wgetstr(self->win,rtn);
    break;
  case 1:
    if (!PyArg_Parse(arg,"i;n", &n))
      return NULL;
    rtn2 = wgetnstr(self->win,rtn,n);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn2 = mvwgetstr(self->win,y,x,rtn);
    break;
  case 3:
    if (!PyArg_Parse(arg,"(iii);y,x,n", &y, &x, &n))
      return NULL;
#ifdef __sgi__
 /* Untested */
    rtn2 = wmove(self->win,y,x)==ERR ? ERR :
      wgetnstr(self->win, rtn, n);
#else
    rtn2 = mvwgetnstr(self->win, y, x, rtn, n);
#endif
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getstr requires 0 to 2 arguments");
    return NULL;
  }
  if (rtn2 == ERR)
    rtn[0] = 0;
  return PyString_FromString(rtn);
}

static PyObject *
PyCursesWindow_Hline(self, args)
     PyCursesWindowObject *self;
     PyObject *args;
{
  PyObject *temp;
  chtype ch;
  int n, x, y, code = OK;
  attr_t attr = A_NORMAL;

  switch (ARG_COUNT(args)) {
  case 2:
    if (!PyArg_Parse(args, "(Oi);ch or int,n", &temp, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_Parse(args, "(Oil);ch or int,n,attr", &temp, &n, &attr))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(args, "(iiOi);y,x,ch o int,n", &y, &x, &temp, &n))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  case 5:
    if (!PyArg_Parse(args, "(iiOil); y,x,ch or int,n,attr", 
		     &y, &x, &temp, &n, &attr))
      return NULL;
    code = wmove(self->win, y, x);
  default:
    PyErr_SetString(PyExc_TypeError, "hline requires 2 or 5 arguments");
    return NULL;
  }

  if (code != ERR) {
    if (!PyCurses_ConvertToChtype(temp, &ch)) {
      PyErr_SetString(PyExc_TypeError, 
		      "argument 1 or 3 must be a ch or an int");
      return NULL;
    }
    return PyCursesCheckERR(whline(self->win, ch | attr, n), "hline");
  } else 
    return PyCursesCheckERR(code, "wmove");
}

static PyObject *
PyCursesWindow_InsCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn, x, y, use_xy = FALSE;
  PyObject *temp;
  chtype ch = 0;
  attr_t attr = A_NORMAL;
  
  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg, "O;ch or int", &temp))
      return NULL;
    break;
  case 2:
    if (!PyArg_Parse(arg, "(Ol);ch or int,attr", &temp, &attr))
      return NULL;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(iiO);y,x,ch or int", &y, &x, &temp))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iiOl);y,x,ch or int, attr", &y, &x, &temp, &attr))
      return NULL;
    use_xy = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "insch requires 1 or 4 arguments");
    return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, 
		    "argument 1 or 3 must be a ch or an int");
    return NULL;
  }
  
  if (use_xy == TRUE)
    rtn = mvwinsch(self->win,y,x, ch | attr);
  else {
    rtn = winsch(self->win, ch | attr);
  }
  return PyCursesCheckERR(rtn, "insch");
}

static PyObject *
PyCursesWindow_InCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y, rtn;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = winch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn = mvwinch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "inch requires 0 or 2 arguments");
    return NULL;
  }
  return PyInt_FromLong((long) rtn);
}

static PyObject *
PyCursesWindow_InStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y, n;
  char rtn[1024]; /* This should be big enough.. I hope */
  int rtn2;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn2 = winstr(self->win,rtn);
    break;
  case 1:
    if (!PyArg_Parse(arg,"i;n", &n))
      return NULL;
    rtn2 = winnstr(self->win,rtn,n);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn2 = mvwinstr(self->win,y,x,rtn);
    break;
  case 3:
    if (!PyArg_Parse(arg, "(iii);y,x,n", &y, &x, &n))
      return NULL;
    rtn2 = mvwinnstr(self->win, y, x, rtn, n);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "instr requires 0 or 3 arguments");
    return NULL;
  }
  if (rtn2 == ERR)
    rtn[0] = 0;
  return PyString_FromString(rtn);
}

static PyObject *
PyCursesWindow_InsStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"s;str", &str))
      return NULL;
    break;
  case 2:
    if (!PyArg_Parse(arg,"(sl);str,attr", &str, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(iis);y,x,str", &y, &x, &str))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iisl);y,x,str,attr", &y, &x, &str, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "insstr requires 1 to 4 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwinsstr(self->win,y,x,str);
  else
    rtn = winsstr(self->win,str);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "insstr");
}

static PyObject *
PyCursesWindow_InsNStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn, x, y, n;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(si);str,n", &str, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(sil);str,n,attr", &str, &n, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iisi);y,x,str,n", &y, &x, &str, &n))
      return NULL;
    use_xy = TRUE;
    break;
  case 5:
    if (!PyArg_Parse(arg,"(iisil);y,x,str,n,attr", &y, &x, &str, &n, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "insnstr requires 2 to 5 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwinsnstr(self->win,y,x,str,n);
  else
    rtn = winsnstr(self->win,str,n);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "insnstr");
}

static PyObject *
PyCursesWindow_Is_LineTouched(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int line, erg;
  if (!PyArg_Parse(arg,"i;line", &line))
    return NULL;
  erg = is_linetouched(self->win, line);
  if (erg == ERR) {
    PyErr_SetString(PyExc_TypeError, 
		    "is_linetouched: line number outside of boundaries");
    return NULL;
  } else 
    if (erg == FALSE) {
      Py_INCREF(Py_False);
      return Py_False;
    } else {
      Py_INCREF(Py_True);
      return Py_True;
    }
}

static PyObject *
PyCursesWindow_NoOutRefresh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol;

  if (self->win->_flags & _ISPAD) {
    switch(ARG_COUNT(arg)) {
    case 6:
      if (!PyArg_Parse(arg, 
		       "(iiiiii);" \
		       "pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol", 
		       &pminrow, &pmincol, &sminrow, 
		       &smincol, &smaxrow, &smaxcol))
	return NULL;
      return PyCursesCheckERR(pnoutrefresh(self->win,
				       pminrow, pmincol, sminrow, 
				       smincol, smaxrow, smaxcol),
			      "pnoutrefresh");
    default:
      PyErr_SetString(PyCursesError, 
		      "noutrefresh was called for a pad;" \
		      "requires 6 arguments");
      return NULL;
    }
  } else {
    if (!PyArg_NoArgs(arg))
      return NULL;    
    return PyCursesCheckERR(wnoutrefresh(self->win), "wnoutrefresh");
  }
}

static PyObject *
PyCursesWindow_PutWin(self, arg)
     PyCursesWindowObject *self;
     PyObject *arg;
{
  PyObject *temp;
  
  if (!PyArg_Parse(arg, "O;fileobj", &temp))
    return NULL;
  if (!PyFile_Check(temp)) {
    PyErr_SetString(PyExc_TypeError, "argument must be a file object");
    return NULL;
  }
  return PyCursesCheckERR(putwin(self->win, PyFile_AsFile(temp)), 
			  "putwin");
}

static PyObject *
PyCursesWindow_RedrawLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int beg, num;
  if (!PyArg_Parse(arg,"(ii);beg,num", &beg, &num))
    return NULL;
  return PyCursesCheckERR(wredrawln(self->win,beg,num), "redrawln");
}

static PyObject *
PyCursesWindow_Refresh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol;
  
  if (self->win->_flags & _ISPAD) {
    switch(ARG_COUNT(arg)) {
    case 6:
      if (!PyArg_Parse(arg, 
		       "(iiiiii);" \
		       "pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol", 
		       &pminrow, &pmincol, &sminrow, 
		       &smincol, &smaxrow, &smaxcol))
	return NULL;
      return PyCursesCheckERR(prefresh(self->win,
				       pminrow, pmincol, sminrow, 
				       smincol, smaxrow, smaxcol),
			      "prefresh");
    default:
      PyErr_SetString(PyCursesError, 
		      "refresh was called for a pad; requires 6 arguments");
      return NULL;
    }
  } else {
    if (!PyArg_NoArgs(arg))
      return NULL;    
    return PyCursesCheckERR(wrefresh(self->win), "wrefresh");
  }
}

static PyObject *
PyCursesWindow_SetScrollRegion(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_Parse(arg,"(ii);top, bottom",&y,&x))
    return NULL;
  return PyCursesCheckERR(wsetscrreg(self->win,y,x), "wsetscrreg");
}

static PyObject *
PyCursesWindow_SubWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;

  if (!PyArg_Parse(arg, "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
    return NULL;

  if (self->win->_flags & _ISPAD)
    win = subpad(self->win, nlines, ncols, begin_y, begin_x);
  else
    win = subwin(self->win,nlines,ncols,begin_y,begin_x);

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }
  
  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCursesWindow_Scroll(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int lines;
  switch(ARG_COUNT(arg)) {
  case 0:
    return PyCursesCheckERR(scroll(self->win), "scroll");
    break;
  case 1:
    if (!PyArg_Parse(arg, "i;lines", &lines))
      return NULL;
    return PyCursesCheckERR(wscrl(self->win, lines), "scroll");
  default:
    PyErr_SetString(PyExc_TypeError, "scroll requires 0 or 1 arguments");
    return NULL;
  }
}

static PyObject *
PyCursesWindow_TouchLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int st, cnt, val;
  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(ii);start,count",&st,&cnt))
      return NULL;
    return PyCursesCheckERR(touchline(self->win,st,cnt), "touchline");
    break;
  case 3:
    if (!PyArg_Parse(arg, "(iii);start,count,val", &st, &cnt, &val))
      return NULL;
    return PyCursesCheckERR(wtouchln(self->win, st, cnt, val), "touchline");
  default:
    PyErr_SetString(PyExc_TypeError, "touchline requires 2 or 3 arguments");
    return NULL;
  }
}

static PyObject *
PyCursesWindow_Vline(self, args)
     PyCursesWindowObject *self;
     PyObject *args;
{
  PyObject *temp;
  chtype ch;
  int n, x, y, code = OK;
  attr_t attr = A_NORMAL;

  switch (ARG_COUNT(args)) {
  case 2:
    if (!PyArg_Parse(args, "(Oi);ch or int,n", &temp, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_Parse(args, "(Oil);ch or int,n,attr", &temp, &n, &attr))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(args, "(iiOi);y,x,ch o int,n", &y, &x, &temp, &n))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  case 5:
    if (!PyArg_Parse(args, "(iiOil); y,x,ch or int,n,attr", 
		     &y, &x, &temp, &n, &attr))
      return NULL;
    code = wmove(self->win, y, x);
  default:
    PyErr_SetString(PyExc_TypeError, "vline requires 2 or 5 arguments");
    return NULL;
  }

  if (code != ERR) {
    if (!PyCurses_ConvertToChtype(temp, &ch)) {
      PyErr_SetString(PyExc_TypeError, 
		      "argument 1 or 3 must be a ch or an int");
      return NULL;
    }
    return PyCursesCheckERR(whline(self->win, ch | attr, n), "vline");
  } else
    return PyCursesCheckERR(code, "wmove");
}

static PyMethodDef PyCursesWindow_Methods[] = {
	{"addch",           (PyCFunction)PyCursesWindow_AddCh},
	{"addnstr",         (PyCFunction)PyCursesWindow_AddNStr},
	{"addstr",          (PyCFunction)PyCursesWindow_AddStr},
	{"attron",          (PyCFunction)PyCursesWindow_wattron},
	{"attr_on",         (PyCFunction)PyCursesWindow_wattron},
	{"attroff",         (PyCFunction)PyCursesWindow_wattroff},
	{"attr_off",        (PyCFunction)PyCursesWindow_wattroff},
	{"attrset",         (PyCFunction)PyCursesWindow_wattrset},
	{"attr_set",        (PyCFunction)PyCursesWindow_wattrset},
	{"bkgd",            (PyCFunction)PyCursesWindow_Bkgd},
	{"bkgdset",         (PyCFunction)PyCursesWindow_BkgdSet},
	{"border",          (PyCFunction)PyCursesWindow_Border, METH_VARARGS},
	{"box",             (PyCFunction)PyCursesWindow_Box},
	{"clear",           (PyCFunction)PyCursesWindow_wclear},
	{"clearok",         (PyCFunction)PyCursesWindow_clearok},
	{"clrtobot",        (PyCFunction)PyCursesWindow_wclrtobot},
	{"clrtoeol",        (PyCFunction)PyCursesWindow_wclrtoeol},
	{"cursyncup",       (PyCFunction)PyCursesWindow_wcursyncup},
	{"delch",           (PyCFunction)PyCursesWindow_DelCh},
	{"deleteln",        (PyCFunction)PyCursesWindow_wdeleteln},
	{"derwin",          (PyCFunction)PyCursesWindow_DerWin},
	{"echochar",        (PyCFunction)PyCursesWindow_EchoChar},
	{"erase",           (PyCFunction)PyCursesWindow_werase},
	{"getbegyx",        (PyCFunction)PyCursesWindow_getbegyx},
	{"getbkgd",         (PyCFunction)PyCursesWindow_GetBkgd},
	{"getch",           (PyCFunction)PyCursesWindow_GetCh},
	{"getkey",          (PyCFunction)PyCursesWindow_GetKey},
	{"getmaxyx",        (PyCFunction)PyCursesWindow_getmaxyx},
	{"getparyx",        (PyCFunction)PyCursesWindow_getparyx},
	{"getstr",          (PyCFunction)PyCursesWindow_GetStr},
	{"getyx",           (PyCFunction)PyCursesWindow_getyx},
	{"hline",           (PyCFunction)PyCursesWindow_Hline},
	{"idlok",           (PyCFunction)PyCursesWindow_idlok},
	{"idcok",           (PyCFunction)PyCursesWindow_idcok},
	{"immedok",         (PyCFunction)PyCursesWindow_immedok},
	{"inch",            (PyCFunction)PyCursesWindow_InCh},
	{"insch",           (PyCFunction)PyCursesWindow_InsCh},
	{"insdelln",        (PyCFunction)PyCursesWindow_winsdelln},
	{"insertln",        (PyCFunction)PyCursesWindow_winsertln},
	{"insnstr",         (PyCFunction)PyCursesWindow_InsNStr},
	{"insstr",          (PyCFunction)PyCursesWindow_InsStr},
	{"instr",           (PyCFunction)PyCursesWindow_InStr},
	{"is_linetouched",  (PyCFunction)PyCursesWindow_Is_LineTouched},
	{"is_wintouched",   (PyCFunction)PyCursesWindow_is_wintouched},
	{"keypad",          (PyCFunction)PyCursesWindow_keypad},
	{"leaveok",         (PyCFunction)PyCursesWindow_leaveok},
	{"move",            (PyCFunction)PyCursesWindow_wmove},
	{"mvwin",           (PyCFunction)PyCursesWindow_mvwin},
	{"mvderwin",        (PyCFunction)PyCursesWindow_mvderwin},
	{"nodelay",         (PyCFunction)PyCursesWindow_nodelay},
	{"noutrefresh",     (PyCFunction)PyCursesWindow_NoOutRefresh},
	{"notimeout",       (PyCFunction)PyCursesWindow_notimeout},
	{"putwin",          (PyCFunction)PyCursesWindow_PutWin},
	{"redrawwin",       (PyCFunction)PyCursesWindow_redrawwin},
	{"redrawln",        (PyCFunction)PyCursesWindow_RedrawLine},
	{"refresh",         (PyCFunction)PyCursesWindow_Refresh},
#ifndef __sgi__
	{"resize",          (PyCFunction)PyCursesWindow_wresize},
#endif
	{"scroll",          (PyCFunction)PyCursesWindow_Scroll},
	{"scrollok",        (PyCFunction)PyCursesWindow_scrollok},
	{"setscrreg",       (PyCFunction)PyCursesWindow_SetScrollRegion},
	{"standend",        (PyCFunction)PyCursesWindow_wstandend},
	{"standout",        (PyCFunction)PyCursesWindow_wstandout},
	{"subpad",          (PyCFunction)PyCursesWindow_SubWin},
	{"subwin",          (PyCFunction)PyCursesWindow_SubWin},
	{"syncdown",        (PyCFunction)PyCursesWindow_wsyncdown},
	{"syncok",          (PyCFunction)PyCursesWindow_syncok},
	{"syncup",          (PyCFunction)PyCursesWindow_wsyncup},
	{"touchline",       (PyCFunction)PyCursesWindow_TouchLine},
	{"touchwin",        (PyCFunction)PyCursesWindow_touchwin},
	{"untouchwin",      (PyCFunction)PyCursesWindow_untouchwin},
	{"vline",           (PyCFunction)PyCursesWindow_Vline},
	{NULL,		        NULL}   /* sentinel */
};

static PyObject *
PyCursesWindow_GetAttr(self, name)
	PyCursesWindowObject *self;
	char *name;
{
  return Py_FindMethod(PyCursesWindow_Methods, (PyObject *)self, name);
}

/* -------------------------------------------------------*/

PyTypeObject PyCursesWindow_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"curses window",	/*tp_name*/
	sizeof(PyCursesWindowObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)PyCursesWindow_Dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)PyCursesWindow_GetAttr, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/*********************************************************************
 Global Functions
**********************************************************************/

static PyObject *ModDict;

/* Function Prototype Macros - They are ugly but very, very useful. ;-)

   X - function name
   TYPE - parameter Type
   ERGSTR - format string for construction of the return value
   PARSESTR - format string for argument parsing
   */

#define NoArgNoReturnFunction(X) \
static PyObject *PyCurses_ ## X (self, arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
  PyCursesInitialised \
  if (!PyArg_NoArgs(arg)) return NULL; \
  return PyCursesCheckERR(X(), # X); }

#define NoArgOrFlagNoReturnFunction(X) \
static PyObject *PyCurses_ ## X (self, arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
  int flag = 0; \
  PyCursesInitialised \
  switch(ARG_COUNT(arg)) { \
  case 0: \
    return PyCursesCheckERR(X(), # X); \
  case 1: \
    if (!PyArg_Parse(arg, "i;True(1) or False(0)", &flag)) return NULL; \
    if (flag) return PyCursesCheckERR(X(), # X); \
    else return PyCursesCheckERR(no ## X (), # X); \
  default: \
    PyErr_SetString(PyExc_TypeError, # X " requires 0 or 1 argument"); \
    return NULL; } }

#define NoArgReturnIntFunction(X) \
static PyObject *PyCurses_ ## X (self, arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
 PyCursesInitialised \
 if (!PyArg_NoArgs(arg)) return NULL; \
 return PyInt_FromLong((long) X()); }


#define NoArgReturnStringFunction(X) \
static PyObject *PyCurses_ ## X (self, arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
  PyCursesInitialised \
  if (!PyArg_NoArgs(arg)) return NULL; \
  return PyString_FromString(X()); }

#define NoArgTrueFalseFunction(X) \
static PyObject * PyCurses_ ## X (self,arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
  PyCursesInitialised \
  if (!PyArg_NoArgs(arg)) return NULL; \
  if (X () == FALSE) { \
    Py_INCREF(Py_False); \
    return Py_False; \
  } \
  Py_INCREF(Py_True); \
  return Py_True; }

#define NoArgNoReturnVoidFunction(X) \
static PyObject * PyCurses_ ## X (self,arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
  PyCursesInitialised \
  if (!PyArg_NoArgs(arg)) return NULL; \
  X(); \
  Py_INCREF(Py_None); \
  return Py_None; }

#define TwoArgNoReturnFunction(X, TYPE, PARSESTR) \
static PyObject * PyCurses_ ## X (self,arg) \
     PyObject * self; \
     PyObject * arg; \
{ \
  TYPE arg1, arg2; \
  PyCursesInitialised \
  if (!PyArg_Parse(arg,PARSESTR, &arg1, &arg2)) return NULL; \
  Py_INCREF(Py_None); \
  return PyCursesCheckERR(X(arg1, arg2), # X); }

NoArgNoReturnFunction(beep)
NoArgNoReturnFunction(def_prog_mode)
NoArgNoReturnFunction(def_shell_mode)
NoArgNoReturnFunction(doupdate)
NoArgNoReturnFunction(endwin)
NoArgNoReturnFunction(flash)
NoArgNoReturnFunction(nocbreak)
NoArgNoReturnFunction(noecho)
NoArgNoReturnFunction(nonl)
NoArgNoReturnFunction(noraw)
NoArgNoReturnFunction(reset_prog_mode)
NoArgNoReturnFunction(reset_shell_mode)
NoArgNoReturnFunction(resetty)
NoArgNoReturnFunction(savetty)

TwoArgNoReturnFunction(resizeterm, int, "(ii);y,x")

NoArgOrFlagNoReturnFunction(cbreak)
NoArgOrFlagNoReturnFunction(echo)
NoArgOrFlagNoReturnFunction(nl)
NoArgOrFlagNoReturnFunction(raw)

NoArgReturnIntFunction(baudrate)
NoArgReturnIntFunction(termattrs)

NoArgReturnStringFunction(termname)
NoArgReturnStringFunction(longname)

NoArgTrueFalseFunction(can_change_color)
NoArgTrueFalseFunction(has_colors)
NoArgTrueFalseFunction(has_ic)
NoArgTrueFalseFunction(has_il)
NoArgTrueFalseFunction(isendwin)

NoArgNoReturnVoidFunction(filter)
NoArgNoReturnVoidFunction(flushinp)
NoArgNoReturnVoidFunction(noqiflush)

static PyObject *
PyCurses_Color_Content(self, arg)
     PyObject * self;
     PyObject * arg;
{
  short color,r,g,b;

  PyCursesInitialised
  PyCursesInitialisedColor

  if (ARG_COUNT(arg) != 1) {
    PyErr_SetString(PyExc_TypeError, 
		    "color_content requires 1 argument");
    return NULL;
  }

  if (!PyArg_Parse(arg, "h;color", &color)) return NULL;

  if (color_content(color, &r, &g, &b) != ERR)
    return Py_BuildValue("(iii)", r, g, b);
  else {
    PyErr_SetString(PyCursesError, 
		    "Argument 1 was out of range. Check value of COLORS.");
    return NULL;
  }
}

static PyObject *
PyCurses_COLOR_PAIR(self, arg)
     PyObject * self;
     PyObject * arg;
{
  int n;

  PyCursesInitialised
  PyCursesInitialisedColor

  if (ARG_COUNT(arg)!=1) {
    PyErr_SetString(PyExc_TypeError, "COLOR_PAIR requires 1 argument");
    return NULL;
  }
  if (!PyArg_Parse(arg, "i;number", &n)) return NULL;
  return PyInt_FromLong((long) (n << 8));
}

static PyObject *
PyCurses_Curs_Set(self, arg)
     PyObject * self;
     PyObject * arg;
{
  int vis,erg;

  PyCursesInitialised

  if (ARG_COUNT(arg)==1) {
    PyErr_SetString(PyExc_TypeError, "curs_set requires 1 argument");
    return NULL;
  }

  if (!PyArg_Parse(arg, "i;int", &vis)) return NULL;

  erg = curs_set(vis);
  if (erg == ERR) return PyCursesCheckERR(erg, "curs_set");

  return PyInt_FromLong((long) erg);
}

static PyObject *
PyCurses_Delay_Output(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ms;

  PyCursesInitialised

  if (ARG_COUNT(arg)==1) {
    PyErr_SetString(PyExc_TypeError, "delay_output requires 1 argument");
    return NULL;
  }
  if (!PyArg_Parse(arg, "i;ms", &ms)) return NULL;

  return PyCursesCheckERR(delay_output(ms), "delay_output");
}

static PyObject *
PyCurses_EraseChar(self,arg)
     PyObject * self;
     PyObject * arg;
{
  char ch;

  PyCursesInitialised

  if (!PyArg_NoArgs(arg)) return NULL;

  ch = erasechar();

  return PyString_FromString(&ch);
}

static PyObject *
PyCurses_getsyx(self, arg)
     PyObject * self;
     PyObject * arg;
{
  int x,y;

  PyCursesInitialised

  if (!PyArg_NoArgs(arg)) return NULL;

  getsyx(y, x);

  return Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCurses_GetWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  WINDOW *win;
  PyObject *temp;

  PyCursesInitialised

  if (!PyArg_Parse(arg, "O;fileobj", &temp)) return NULL;

  if (!PyFile_Check(temp)) {
    PyErr_SetString(PyExc_TypeError, "argument must be a file object");
    return NULL;
  }

  win = getwin(PyFile_AsFile(temp));

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return PyCursesWindow_New(win);
}

static PyObject *
PyCurses_HalfDelay(self,arg)
     PyObject * self;
     PyObject * arg;
{
  unsigned char tenths;

  PyCursesInitialised

  switch(ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg, "b;tenths", &tenths)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "halfdelay requires 1 argument");
    return NULL;
  }

  return PyCursesCheckERR(halfdelay(tenths), "halfdelay");
}

#ifndef __sgi__
 /* No has_key! */
static PyObject * PyCurses_has_key(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;

  PyCursesInitialised

  if (!PyArg_Parse(arg,"i",&ch)) return NULL;

  if (has_key(ch) == FALSE) {
    Py_INCREF(Py_False);
    return Py_False;
  }
  Py_INCREF(Py_True);
  return Py_True; 
}
#endif

static PyObject *
PyCurses_Init_Color(self, arg)
     PyObject * self;
     PyObject * arg;
{
  short color, r, g, b;

  PyCursesInitialised
  PyCursesInitialisedColor

  switch(ARG_COUNT(arg)) {
  case 4:
    if (!PyArg_Parse(arg, "(hhhh);color,r,g,b", &color, &r, &g, &b)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "init_color requires 4 arguments");
    return NULL;
  }

  return PyCursesCheckERR(init_color(color, r, g, b), "init_color");
}

static PyObject *
PyCurses_Init_Pair(self, arg)
     PyObject * self;
     PyObject * arg;
{
  short pair, f, b;

  PyCursesInitialised
  PyCursesInitialisedColor

  if (ARG_COUNT(arg) != 3) {
    PyErr_SetString(PyExc_TypeError, "init_pair requires 3 arguments");
    return NULL;
  }

  if (!PyArg_Parse(arg, "(hhh);pair, f, b", &pair, &f, &b)) return NULL;

  return PyCursesCheckERR(init_pair(pair, f, b), "init_pair");
}

static PyObject * 
PyCurses_InitScr(self, args)
     PyObject * self;
     PyObject * args;
{
  WINDOW *win;
  PyObject *lines, *cols;

  if (!PyArg_NoArgs(args)) return NULL;

  if (initialised == TRUE) {
    wrefresh(stdscr);
    return (PyObject *)PyCursesWindow_New(stdscr);
  }

  win = initscr();

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  initialised = TRUE;

  lines = PyInt_FromLong((long) LINES);
  PyDict_SetItemString(ModDict, "LINES", lines);
  Py_DECREF(lines);
  cols = PyInt_FromLong((long) COLS);
  PyDict_SetItemString(ModDict, "COLS", cols);
  Py_DECREF(cols);

  return (PyObject *)PyCursesWindow_New(win);
}


static PyObject *
PyCurses_IntrFlush(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;

  PyCursesInitialised

  switch(ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "intrflush requires 1 argument");
    return NULL;
  }

  return PyCursesCheckERR(intrflush(NULL,ch), "intrflush");
}

static PyObject *
PyCurses_KeyName(self,arg)
     PyObject * self;
     PyObject * arg;
{
  const char *knp;
  int ch;

  PyCursesInitialised

  if (!PyArg_Parse(arg,"i",&ch)) return NULL;

  knp = keyname(ch);

  return PyString_FromString((knp == NULL) ? "" : (char *)knp);
}

static PyObject *  
PyCurses_KillChar(self,arg)  
PyObject * self;  
PyObject * arg;  
{  
  char ch;  

  if (!PyArg_NoArgs(arg)) return NULL;  

  ch = killchar();  

  return PyString_FromString(&ch);  
}  

static PyObject *
PyCurses_Meta(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;

  PyCursesInitialised

  switch(ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "meta requires 1 argument");
    return NULL;
  }

  return PyCursesCheckERR(meta(stdscr, ch), "meta");
}

static PyObject *
PyCurses_NewPad(self,arg)
     PyObject * self;
     PyObject * arg;
{
  WINDOW *win;
  int nlines, ncols;

  PyCursesInitialised 

  if (!PyArg_Parse(arg,"(ii);nlines,ncols",&nlines,&ncols)) return NULL;

  win = newpad(nlines, ncols);
  
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_NewWindow(self,arg)
     PyObject * self;
     PyObject * arg;
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;

  PyCursesInitialised

  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(ii);nlines,ncols",&nlines,&ncols))
      return NULL;
    win = newpad(nlines, ncols);
    break;
  case 4:
    if (!PyArg_Parse(arg, "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    win = newwin(nlines,ncols,begin_y,begin_x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "newwin requires 2 or 4 arguments");
    return NULL;
  }

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_Pair_Content(self, arg)
     PyObject * self;
     PyObject * arg;
{
  short pair,f,b;

  PyCursesInitialised
  PyCursesInitialisedColor

  switch(ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg, "h;pair", &pair)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "pair_content requires 1 argument");
    return NULL;
  }

  if (!pair_content(pair, &f, &b)) {
    PyErr_SetString(PyCursesError,
		    "Argument 1 was out of range. (1..COLOR_PAIRS-1)");
    return NULL;
  }

  return Py_BuildValue("(ii)", f, b);
}

static PyObject *
PyCurses_PAIR_NUMBER(self, arg)
     PyObject * self;
     PyObject * arg;
{
  int n;

  PyCursesInitialised
  PyCursesInitialisedColor

  switch(ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg, "i;pairvalue", &n)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError,
                    "PAIR_NUMBER requires 1 argument");
    return NULL;
  }

  return PyInt_FromLong((long) ((n & A_COLOR) >> 8));
}

static PyObject *
PyCurses_Putp(self,arg)
     PyObject *self;
     PyObject *arg;
{
  char *str;

  if (!PyArg_Parse(arg,"s;str", &str)) return NULL;
  return PyCursesCheckERR(putp(str), "putp");
}

static PyObject *
PyCurses_QiFlush(self, arg)
     PyObject * self;
     PyObject * arg;
{
  int flag = 0;

  PyCursesInitialised

  switch(ARG_COUNT(arg)) {
  case 0:
    qiflush();
    Py_INCREF(Py_None);
    return Py_None;
  case 1:
    if (!PyArg_Parse(arg, "i;True(1) or False(0)", &flag)) return NULL;
    if (flag) qiflush();
    else noqiflush();
    Py_INCREF(Py_None);
    return Py_None;
  default:
    PyErr_SetString(PyExc_TypeError, "nl requires 0 or 1 argument");
    return NULL;
  }
}

static PyObject *
PyCurses_setsyx(self, arg)
     PyObject * self;
     PyObject * arg;
{
  int y,x;

  PyCursesInitialised

  if (ARG_COUNT(arg)!=3) {
    PyErr_SetString(PyExc_TypeError, "curs_set requires 3 argument");
    return NULL;
  }

  if (!PyArg_Parse(arg, "(ii);y, x", &y, &x)) return NULL;

  setsyx(y,x);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCurses_Start_Color(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int code;
  PyObject *c, *cp;

  PyCursesInitialised

  if (!PyArg_NoArgs(arg)) return NULL;

  code = start_color();
  if (code != ERR) {
    initialisedcolors = TRUE;
    c = PyInt_FromLong((long) COLORS);
    PyDict_SetItemString(ModDict, "COLORS", c);
    Py_DECREF(c);
    cp = PyInt_FromLong((long) COLOR_PAIRS);
    PyDict_SetItemString(ModDict, "COLOR_PAIRS", cp);
    Py_DECREF(cp);
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    PyErr_SetString(PyCursesError, "start_color() returned ERR");
    return NULL;
  }
}

static PyObject *
PyCurses_UnCtrl(self,arg)
     PyObject * self;
     PyObject * arg;
{
  PyObject *temp;
  chtype ch;

  PyCursesInitialised

  if (!PyArg_Parse(arg,"O;ch or int",&temp)) return NULL;

  if (PyInt_Check(temp))
    ch = (chtype) PyInt_AsLong(temp);
  else if (PyString_Check(temp))
    ch = (chtype) *PyString_AsString(temp);
  else {
    PyErr_SetString(PyExc_TypeError, "argument must be a ch or an int");
    return NULL;
  }

  return PyString_FromString(unctrl(ch));
}

static PyObject *
PyCurses_UngetCh(self,arg)
     PyObject * self;
     PyObject * arg;
{
  PyObject *temp;
  chtype ch;

  PyCursesInitialised

  if (!PyArg_Parse(arg,"O;ch or int",&temp)) return NULL;

  if (PyInt_Check(temp))
    ch = (chtype) PyInt_AsLong(temp);
  else if (PyString_Check(temp))
    ch = (chtype) *PyString_AsString(temp);
  else {
    PyErr_SetString(PyExc_TypeError, "argument must be a ch or an int");
    return NULL;
  }

  return PyCursesCheckERR(ungetch(ch), "ungetch");
}

static PyObject *
PyCurses_Use_Env(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int flag;

  PyCursesInitialised

  switch(ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"i;True(1), False(0)",&flag))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "use_env requires 1 argument");
    return NULL;
  }
  use_env(flag);
  Py_INCREF(Py_None);
  return Py_None;
}

/* List of functions defined in the module */

static PyMethodDef PyCurses_methods[] = {
  {"baudrate",            (PyCFunction)PyCurses_baudrate},
  {"beep",                (PyCFunction)PyCurses_beep},
  {"can_change_color",    (PyCFunction)PyCurses_can_change_color},
  {"cbreak",              (PyCFunction)PyCurses_cbreak},
  {"color_content",       (PyCFunction)PyCurses_Color_Content},
  {"COLOR_PAIR",          (PyCFunction)PyCurses_COLOR_PAIR},
  {"curs_set",            (PyCFunction)PyCurses_Curs_Set},
  {"def_prog_mode",       (PyCFunction)PyCurses_def_prog_mode},
  {"def_shell_mode",      (PyCFunction)PyCurses_def_shell_mode},
  {"delay_output",        (PyCFunction)PyCurses_Delay_Output},
  {"doupdate",            (PyCFunction)PyCurses_doupdate},
  {"echo",                (PyCFunction)PyCurses_echo},
  {"endwin",              (PyCFunction)PyCurses_endwin},
  {"erasechar",           (PyCFunction)PyCurses_EraseChar},
  {"filter",              (PyCFunction)PyCurses_filter},
  {"flash",               (PyCFunction)PyCurses_flash},
  {"flushinp",            (PyCFunction)PyCurses_flushinp},
  {"getsyx",              (PyCFunction)PyCurses_getsyx},
  {"getwin",              (PyCFunction)PyCurses_GetWin},
  {"has_colors",          (PyCFunction)PyCurses_has_colors},
  {"has_ic",              (PyCFunction)PyCurses_has_ic},
  {"has_il",              (PyCFunction)PyCurses_has_il},
#ifndef __sgi__
  {"has_key",             (PyCFunction)PyCurses_has_key},
#endif
  {"halfdelay",           (PyCFunction)PyCurses_HalfDelay},
  {"init_color",          (PyCFunction)PyCurses_Init_Color},
  {"init_pair",           (PyCFunction)PyCurses_Init_Pair},
  {"initscr",             (PyCFunction)PyCurses_InitScr},
  {"intrflush",           (PyCFunction)PyCurses_IntrFlush},
  {"isendwin",            (PyCFunction)PyCurses_isendwin},
  {"keyname",             (PyCFunction)PyCurses_KeyName},
  {"killchar",            (PyCFunction)PyCurses_KillChar}, 
  {"longname",            (PyCFunction)PyCurses_longname}, 
  {"meta",                (PyCFunction)PyCurses_Meta},
  {"newpad",              (PyCFunction)PyCurses_NewPad},
  {"newwin",              (PyCFunction)PyCurses_NewWindow},
  {"nl",                  (PyCFunction)PyCurses_nl},
  {"nocbreak",            (PyCFunction)PyCurses_nocbreak},
  {"noecho",              (PyCFunction)PyCurses_noecho},
  {"nonl",                (PyCFunction)PyCurses_nonl},
  {"noqiflush",           (PyCFunction)PyCurses_noqiflush},
  {"noraw",               (PyCFunction)PyCurses_noraw},
  {"pair_content",        (PyCFunction)PyCurses_Pair_Content},
  {"PAIR_NUMBER",         (PyCFunction)PyCurses_PAIR_NUMBER},
  {"putp",                (PyCFunction)PyCurses_Putp},
  {"qiflush",             (PyCFunction)PyCurses_QiFlush},
  {"raw",                 (PyCFunction)PyCurses_raw},
  {"reset_prog_mode",     (PyCFunction)PyCurses_reset_prog_mode},
  {"reset_shell_mode",    (PyCFunction)PyCurses_reset_shell_mode},
  {"resetty",             (PyCFunction)PyCurses_resetty},
  {"savetty",             (PyCFunction)PyCurses_savetty},
  {"resizeterm",          (PyCFunction)PyCurses_resizeterm},
  {"setsyx",              (PyCFunction)PyCurses_setsyx},
  {"start_color",         (PyCFunction)PyCurses_Start_Color},
  {"termattrs",           (PyCFunction)PyCurses_termattrs},
  {"termname",            (PyCFunction)PyCurses_termname},
  {"unctrl",              (PyCFunction)PyCurses_UnCtrl},
  {"ungetch",             (PyCFunction)PyCurses_UngetCh},
  {"use_env",             (PyCFunction)PyCurses_Use_Env},
  {NULL,		NULL}		/* sentinel */
};

/* Initialization function for the module */

void
initjack_curses()
{
	PyObject *m, *d, *v;

	/* Create the module and add the functions */
	m = Py_InitModule("jack_curses", PyCurses_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ModDict = d; /* For PyCurses_InitScr */

	/* For exception curses.error */
	PyCursesError = PyString_FromString("curses.error");
	PyDict_SetItemString(d, "error", PyCursesError);

	/* Make the version available */
	v = PyString_FromString(PyCursesVersion);
	PyDict_SetItemString(d, "version", v);
	PyDict_SetItemString(d, "__version__", v);
	Py_DECREF(v);

	/* Here are some attributes you can add to chars to print */
	
#define SetDictInt(string,ch) \
	PyDict_SetItemString(ModDict,string,PyInt_FromLong((long) (ch)));

#ifndef __sgi__
  /* On IRIX 5.3, the ACS characters aren't available until initscr() has been called.  */
        SetDictInt("ACS_ULCORNER",      (ACS_ULCORNER));
	SetDictInt("ACS_LLCORNER",      (ACS_LLCORNER));
	SetDictInt("ACS_URCORNER",      (ACS_URCORNER));
	SetDictInt("ACS_LRCORNER",      (ACS_LRCORNER));
	SetDictInt("ACS_LTEE",          (ACS_LTEE));
	SetDictInt("ACS_RTEE",          (ACS_RTEE));
	SetDictInt("ACS_BTEE",          (ACS_BTEE));
	SetDictInt("ACS_TTEE",          (ACS_TTEE));
	SetDictInt("ACS_HLINE",         (ACS_HLINE));
	SetDictInt("ACS_VLINE",         (ACS_VLINE));
	SetDictInt("ACS_PLUS",          (ACS_PLUS));
	SetDictInt("ACS_S1",            (ACS_S1));
	SetDictInt("ACS_S9",            (ACS_S9));
	SetDictInt("ACS_DIAMOND",       (ACS_DIAMOND));
	SetDictInt("ACS_CKBOARD",       (ACS_CKBOARD));
	SetDictInt("ACS_DEGREE",        (ACS_DEGREE));
	SetDictInt("ACS_PLMINUS",       (ACS_PLMINUS));
	SetDictInt("ACS_BULLET",        (ACS_BULLET));
	SetDictInt("ACS_LARROW",        (ACS_LARROW));
	SetDictInt("ACS_RARROW",        (ACS_RARROW));
	SetDictInt("ACS_DARROW",        (ACS_DARROW));
	SetDictInt("ACS_UARROW",        (ACS_UARROW));
	SetDictInt("ACS_BOARD",         (ACS_BOARD));
	SetDictInt("ACS_LANTERN",       (ACS_LANTERN));
	SetDictInt("ACS_BLOCK",         (ACS_BLOCK));
#ifndef __sgi__
  /* The following are never available on IRIX 5.3 */
	SetDictInt("ACS_S3",            (ACS_S3));
	SetDictInt("ACS_LEQUAL",        (ACS_LEQUAL));
	SetDictInt("ACS_GEQUAL",        (ACS_GEQUAL));
	SetDictInt("ACS_PI",            (ACS_PI));
	SetDictInt("ACS_NEQUAL",        (ACS_NEQUAL));
	SetDictInt("ACS_STERLING",      (ACS_STERLING));
#endif
	SetDictInt("ACS_BSSB",          (ACS_ULCORNER));
	SetDictInt("ACS_SSBB",          (ACS_LLCORNER));
	SetDictInt("ACS_BBSS",          (ACS_URCORNER));
	SetDictInt("ACS_SBBS",          (ACS_LRCORNER));
	SetDictInt("ACS_SBSS",          (ACS_RTEE));
	SetDictInt("ACS_SSSB",          (ACS_LTEE));
	SetDictInt("ACS_SSBS",          (ACS_BTEE));
	SetDictInt("ACS_BSSS",          (ACS_TTEE));
	SetDictInt("ACS_BSBS",          (ACS_HLINE));
	SetDictInt("ACS_SBSB",          (ACS_VLINE));
	SetDictInt("ACS_SSSS",          (ACS_PLUS));
#endif

	SetDictInt("A_ATTRIBUTES",      A_ATTRIBUTES);
	SetDictInt("A_NORMAL",		    A_NORMAL);
	SetDictInt("A_STANDOUT",	    A_STANDOUT);
	SetDictInt("A_UNDERLINE",	    A_UNDERLINE);
	SetDictInt("A_REVERSE",		    A_REVERSE);
	SetDictInt("A_BLINK",		    A_BLINK);
	SetDictInt("A_DIM",		        A_DIM);
	SetDictInt("A_BOLD",		    A_BOLD);
	SetDictInt("A_ALTCHARSET",	    A_ALTCHARSET);
	SetDictInt("A_INVIS",           A_INVIS);
	SetDictInt("A_PROTECT",         A_PROTECT);
#ifndef __sgi__
	SetDictInt("A_HORIZONTAL",      A_HORIZONTAL);
	SetDictInt("A_LEFT",            A_LEFT);
	SetDictInt("A_LOW",             A_LOW);
	SetDictInt("A_RIGHT",           A_RIGHT);
	SetDictInt("A_TOP",             A_TOP);
	SetDictInt("A_VERTICAL",        A_VERTICAL);
#endif
	SetDictInt("A_CHARTEXT",        A_CHARTEXT);
	SetDictInt("A_COLOR",           A_COLOR);
#ifndef __sgi__
	SetDictInt("WA_ATTRIBUTES",     WA_ATTRIBUTES);
	SetDictInt("WA_NORMAL",		    WA_NORMAL);
	SetDictInt("WA_STANDOUT",	    WA_STANDOUT);
	SetDictInt("WA_UNDERLINE",	    WA_UNDERLINE);
	SetDictInt("WA_REVERSE",	    WA_REVERSE);
	SetDictInt("WA_BLINK",		    WA_BLINK);
	SetDictInt("WA_DIM",		    WA_DIM);
	SetDictInt("WA_BOLD",		    WA_BOLD);
	SetDictInt("WA_ALTCHARSET",	    WA_ALTCHARSET);
	SetDictInt("WA_INVIS",          WA_INVIS);
	SetDictInt("WA_PROTECT",        WA_PROTECT);
	SetDictInt("WA_HORIZONTAL",     WA_HORIZONTAL);
	SetDictInt("WA_LEFT",           WA_LEFT);
	SetDictInt("WA_LOW",            WA_LOW);
	SetDictInt("WA_RIGHT",          WA_RIGHT);
	SetDictInt("WA_TOP",            WA_TOP);
	SetDictInt("WA_VERTICAL",       WA_VERTICAL);
#endif
	SetDictInt("COLOR_BLACK",       COLOR_BLACK);
	SetDictInt("COLOR_RED",         COLOR_RED);
	SetDictInt("COLOR_GREEN",       COLOR_GREEN);
	SetDictInt("COLOR_YELLOW",      COLOR_YELLOW);
	SetDictInt("COLOR_BLUE",        COLOR_BLUE);
	SetDictInt("COLOR_MAGENTA",     COLOR_MAGENTA);
	SetDictInt("COLOR_CYAN",        COLOR_CYAN);
	SetDictInt("COLOR_WHITE",       COLOR_WHITE);

	/* Now set everything up for KEY_ variables */
	{
	  int key;
	  char *key_n;
	  char *key_n2;
	  for (key=KEY_MIN;key < KEY_MAX; key++) {
	    key_n = (char *)keyname(key);
	    if (key_n == NULL || strcmp(key_n,"UNKNOWN KEY")==0)
	      continue;
	    if (strncmp(key_n,"KEY_F(",6)==0) {
	      char *p1, *p2;
	      key_n2 = malloc(strlen(key_n)+1);
	      p1 = key_n;
	      p2 = key_n2;
	      while (*p1) {
		if (*p1 != '(' && *p1 != ')') {
		  *p2 = *p1;
		  p2++;
		}
		p1++;
	      }
	      *p2 = (char)0;
	    } else
	      key_n2 = key_n;
	    PyDict_SetItemString(d,key_n2,PyInt_FromLong((long) key));
	    if (key_n2 != key_n)
	      free(key_n2);
	  }
	  SetDictInt("KEY_MIN", KEY_MIN);
	  SetDictInt("KEY_MAX", KEY_MAX);
	}

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module curses");
}


