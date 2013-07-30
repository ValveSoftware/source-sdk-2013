# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
# The initial version of the directory_cache_t class was written
# by Matthias Baas (baas@ira.uka.de).

"""
directory cache implementation.

This module contains the implementation of a cache that uses individual
files, stored in a dedicated cache directory, to store the cached contents.

The :class:`parser.directory_cache_t` class instance could be passed as the `cache`
argument of the :func:`parser.parse` function.
"""

import os, os.path, gzip, hashlib
try:
    import cPickle as pickle
except ImportError:
    import pickle
from . import declarations_cache

class index_entry_t:
    """
    Entry of the index table in the directory cache index.

    Each cached header file (i.e. each .cache file) has a corresponding
    index_entry_t object. This object is used to determine whether the
    cache file with the declarations is still valid or not.

    This class is a helper class for the directory_cache_t class.
    """

    def __init__( self, filesigs, configsig ):
        """
        :param filesigs: a list of tuples( `fileid`, `sig`)...
        :param configsig: the signature of the configuration object.
        """
        self.filesigs = filesigs
        self.configsig = configsig

    def __getstate__(self):
        return (self.filesigs, self.configsig)

    def __setstate__(self, state):
        self.filesigs, self.configsig = state


class directory_cache_t ( declarations_cache.cache_base_t ):
    """cache class that stores its data as multiple files inside a directory.

    The cache stores one index file called `index.dat` which is always
    read by the cache when the cache object is created. Each header file
    will have its corresponding .cache file that stores the declarations
    found in the header file. The index file is used to determine whether
    a .cache file is still valid or not (by checking if one of the dependent
    files (i.e. the header file itself and all included files) have been
    modified since the last run).
    """

    def __init__( self, dir="cache", compression=False, md5_sigs=True ):
        """
        :param dir: cache directory path, it is created, if it does not exist

        :param compression: if `True`, the cache files will be compressed using `gzip`

        :param md5_sigs: `md5_sigs` determines whether file modifications is checked
                         by computing a `md5` digest or by checking the modification date
        """
        declarations_cache.cache_base_t.__init__(self)

        # Cache directory
        self.__dir = os.path.abspath(dir)

        # Flag that determines whether the cache files will be compressed
        self.__compression = compression

        # Flag that determines whether the signature is a md5 digest or
        # the modification time
        # (this flag is passed to the filename_repository_t class)
        self.__md5_sigs = md5_sigs

        # Filename repository
        self.__filename_rep = filename_repository_t(self.__md5_sigs)

        # Index dictionary (Key is the value returned by _create_cache_key()
        # (which is based on the header file name) and value is an
        # index_entry_t object)
        self.__index = {}

        # Flag that indicates whether the index was modified
        self.__modified_flag = False

        # Check if dir refers to an existing file...
        if os.path.isfile(self.__dir):
            raise ValueError("Cannot use %s as cache directory. There is already a file with that name."%self.__dir)

        # Load the cache or create the cache directory...
        if os.path.isdir(self.__dir):
            self._load()
        else:
            # Create the cache directory...
            os.mkdir(self.__dir)

    def flush(self):
        """Save the index table to disk."""
        self._save()

    def update(self, source_file, configuration, declarations, included_files):
        """Replace a cache entry by a new value.

        :param source_file: a C++ source file name.
        :type source_file: str
        :param configuration: configuration  object.
        :type configuration: :class:`config_t`
        :param declarations: declarations contained in the `source_file`
        :type declarations: pickable object
        :param included_files: included files
        :type included_files: list of str
        """
        # Normlize all paths...
        source_file = os.path.normpath(source_file)
        included_files = [os.path.normpath(p) for p in included_files]

        # Create the list of dependent files. This is the included_files list
        # + the source file. Duplicate names are removed.
        dependent_files = {}
        for name in [source_file]+included_files:
            dependent_files[name] = 1
        dependent_files = list(dependent_files.keys())

        key = self._create_cache_key(source_file)
        # Remove an existing entry (if there is one)
        # After calling this method, it is guaranteed that __index[key]
        # does not exist anymore.
        self._remove_entry(source_file, key)

        # Create a new entry...

        # Create the sigs of all dependent files...
        filesigs = []
        for filename in dependent_files:
            id_,sig = self.__filename_rep.acquire_filename(filename)
            filesigs.append((id_,sig))

        configsig = self._create_config_signature(configuration)
        entry = index_entry_t(filesigs, configsig)
        self.__index[key] = entry
        self.__modified_flag = True

        # Write the declarations into the cache file...
        cachefilename = self._create_cache_filename(source_file)
        self._write_file(cachefilename, declarations)


    def cached_value(self, source_file, configuration):
        """Return the cached declarations or None.

        :param source_file: Header file name
        :type source_file: str
        :param configuration: Configuration object
        :type configuration: :class:`parser.config_t`
        :rtype: Cached declarations or None
        """

        # Check if the cache contains an entry for source_file
        key = self._create_cache_key(source_file)
        entry = self.__index.get(key)
        if entry==None:
#            print "CACHE: %s: Not cached"%source_file
            return None

        # Check if the entry is still valid. It is not valid if:
        #  - the source_file has been updated
        #  - the configuration object has changed (i.e. the header is parsed
        #    by gccxml with different settings which may influence the
        #    declarations)
        #  - the included files have been updated
        #    (this list is part of the cache entry as it cannot be known
        #    by the caller when cached_value() is called. It was instead
        #    passed to update())

        # Check if the config is different...
        configsig = self._create_config_signature(configuration)
        if configsig!=entry.configsig:
#            print "CACHE: %s: Config mismatch"%source_file
            return None

        # Check if any of the dependent files has been modified...
        for id_, sig in entry.filesigs:
            if self.__filename_rep.is_file_modified(id_, sig):
#                print "CACHE: %s: Entry not up to date"%source_file
                return None

        # Load and return the cached declarations
        cachefilename = self._create_cache_filename(source_file)
        decls = self._read_file(cachefilename)

#        print "CACHE: Using cached decls for",source_file
        return decls

    def _load(self):
        """Load the cache.

        Loads the `index.dat` file, which contains the index table and the file name repository.

        This method is called by the :meth:`__init__`
        """

        indexfilename = os.path.join(self.__dir, "index.dat")
        if os.path.exists(indexfilename):
            data = self._read_file(indexfilename)
            self.__index = data[0]
            self.__filename_rep = data[1]
            if self.__filename_rep._md5_sigs!=self.__md5_sigs:
                print("CACHE: Warning: md5_sigs stored in the cache is set to %s."%self.__filename_rep._md5_sigs)
                print("       Please remove the cache to change this setting.")
                self.__md5_sigs = self.__filename_rep._md5_sigs
        else:
            self.__index = {}
            self.__filename_rep = filename_repository_t(self.__md5_sigs)

        self.__modified_flag = False

    def _save(self):
        """
        save the cache index, in case it was modified.

        Saves the index table and the file name repository in the file `index.dat`
        """
        if self.__modified_flag:
            self.__filename_rep.update_id_counter()
            indexfilename = os.path.join(self.__dir, "index.dat")
            self._write_file(indexfilename, (self.__index,self.__filename_rep))
            self.__modified_flag = False

    def _read_file(self, filename):
        """
        read a Python object from a cache file.

        Reads a pickled object from disk and returns it.

        :param filename: Name of the file that should be read.
        :type filename: str
        :rtype: object
        """
        if self.__compression:
            f = gzip.GzipFile(filename, "rb")
        else:
            f = open(filename, "rb")
        res = pickle.load(f)
        f.close()
        return res

    def _write_file(self, filename, data):
        """Write a data item into a file.

        The data object is written to a file using the pickle mechanism.

        :param filename: Output file name
        :type filename: str
        :param data: A Python object that will be pickled
        """
        if self.__compression:
            f = gzip.GzipFile(filename, "wb")
        else:
            f = open(filename, "wb")
        pickle.dump(data, f, pickle.HIGHEST_PROTOCOL)
        f.close()

    def _remove_entry(self, source_file, key):
        """Remove an entry from the cache.

        source_file is the name of the header and key is its corresponding
        cache key (obtained by a call to :meth:_create_cache_key ).
        The entry is removed from the index table, any referenced file
        name is released and the cache file is deleted.

        If key references a non-existing entry, the method returns
        immediately.

        :param source_file: Header file name
        :type source_file: str
        :param key: Key value for the specified header file
        :type key: hash table object
        """

        entry = self.__index.get(key)
        if entry==None:
            return

        # Release the referenced files...
        for id_, sig in entry.filesigs:
            self.__filename_rep.release_filename(id_)

        # Remove the cache entry...
        del self.__index[key]
        self.__modified_flag = True

        # Delete the corresponding cache file...
        cachefilename = self._create_cache_filename(source_file)
        try:
            os.remove(cachefilename)
        except OSError as e:
            print("Could not remove cache file (%s)"%e)


    def _create_cache_key(self, source_file):
        """
        return the cache key for a header file.

        :param source_file: Header file name
        :type source_file: str
        :rtype: str
        """
        path, name = os.path.split(source_file)
        return name+str(hash(path))

    def _create_cache_filename(self, source_file):
        """
        return the cache file name for a header file.

        :param source_file: Header file name
        :type source_file: str
        :rtype: str
        """
        res = self._create_cache_key(source_file)+".cache"
        return os.path.join(self.__dir, res)

    def _create_config_signature(self, config):
        """
        return the signature for a config object.

        The signature is computed as md5 digest of the contents of
        working_directory, include_paths, define_symbols and
        undefine_symbols.

        :param config: Configuration object
        :type config: :class:`parser.config_t`
        :rtype: str
        """
        m = hashlib.md5()
        m.update(config.working_directory)
        for p in config.include_paths: m.update(p)
        for p in config.define_symbols: m.update(p)
        for p in config.undefine_symbols: m.update(p)
        for p in config.cflags: m.update(p)
        return m.digest()




class filename_entry_t:
    """This is a record stored in the filename_repository_t class.

    The class is an internal class used in the implementation of the
    filename_repository_t class and it just serves as a container for
    the file name and the reference count.
    """

    def __init__( self, filename ):
        """Constructor.

        The reference count is initially set to 0.
        """
        # Filename
        self.filename = filename
        # Reference count
        self.refcount = 0

        # Cached signature value for the file.
        # If sig_valid flag is False, the signature still has to be computed,
        # otherwise the cached value can be used.
        # These attributes must not be pickled!
        self.sig_valid = False
        self.signature = None

    def __getstate__(self):
        # Only pickle filename and refcount
        return (self.filename, self.refcount)

    def __setstate__(self, state):
        self.filename, self.refcount = state
        self.sig_valid = False
        self.signature = None

    def inc_ref_count(self):
        """Increase the reference count by 1."""
        self.refcount += 1

    def dec_ref_count(self):
        """Decrease the reference count by 1 and return the new count."""
        self.refcount -= 1
        return self.refcount


class filename_repository_t:
    """File name repository.

    This class stores file names and can check whether a file has been
    modified or not since a previous call.
    A file name is stored by calling acquire_filename() which returns
    an ID and a signature of the file. The signature can later be used
    to check if the file was modified by calling is_file_modified().
    If the file name is no longer required release_filename() should be
    called so that the entry can be removed from the repository.
    """

    def __init__( self, md5_sigs ):
        """Constructor.
        """

        # Flag that determines whether the signature is a md5 digest or
        # the modification time
        # (this flag is passed to the filename_repository_t class)
        self._md5_sigs = md5_sigs

        # ID lookup table (key: filename / value: id_)
        self.__id_lut = {}

        # Entry dictionary (key: id_ / value: filename_entry_t)
        # This dictionary contains the actual data.
        # It must always hold that each entry in __entries has a corresponding
        # entry in __id_lut (i.e. the keys in __id_lut must be the names
        # stored in __entries)
        self.__entries = {}

        # A counter for new ids
        self.__next_id = 1

    def acquire_filename(self, name):
        """Acquire a file name and return its id and its signature.
        """
        id_ = self.__id_lut.get(name)
        # Is this a new entry?
        if id_==None:
            # then create one...
            id_ = self.__next_id
            self.__next_id += 1
            self.__id_lut[name] = id_
            entry = filename_entry_t(name)
            self.__entries[id_] = entry
        else:
            # otherwise obtain the entry...
            entry = self.__entries[id_]

        entry.inc_ref_count()
        return id_, self._get_signature(entry)

    def release_filename(self, id_):
        """Release a file name.
        """
        entry = self.__entries.get(id_)
        if entry==None:
            raise ValueError("Invalid filename id (%d)"%id_)

        # Decrease reference count and check if the entry has to be removed...
        if entry.dec_ref_count()==0:
            del self.__entries[id_]
            del self.__id_lut[entry.filename]

    def is_file_modified(self, id_, signature):
        """Check if the file referred to by `id_` has been modified.
        """
        entry = self.__entries.get(id_)
        if entry==None:
            raise ValueError("Invalid filename id_ (%d)"%id_)

        # Is the signature already known?
        if entry.sig_valid:
            # use the cached signature
            filesig = entry.signature
        else:
            # compute the signature and store it
            filesig = self._get_signature(entry)
            entry.signature = filesig
            entry.sig_valid = True

        return filesig!=signature

    def update_id_counter(self):
        """Update the `id_` counter so that it doesn't grow forever.
        """
        if len(self.__entries)==0:
            self.__next_id = 1
        else:
            self.__next_id = max(self.__entries.keys())+1

    def _get_signature(self, entry):
        """Return the signature of the file stored in entry.
        """
        if self._md5_sigs:
            # return md5 digest of the file content...
            if not os.path.exists(entry.filename):
                return None
            try:
                f = open(entry.filename)
            except IOError as e:
                print("Cannot determine md5 digest:",e)
                return None
            data = f.read()
            f.close()
            return hashlib.md5(data).digest()
        else:
            # return file modification date...
            try:
                return os.path.getmtime(entry.filename)
            except OSError as e:
                return None

    def _dump(self):
        """Dump contents for debugging/testing.
        """

        print(70*"-")
        print("ID lookup table:")
        for name in self.__id_lut:
            id_ = self.__id_lut[name]
            print("  %s -> %d"%(name, id_))

        print(70*"-")
        print("%-4s %-60s %s"%("ID", "Filename", "Refcount"))
        print(70*"-")
        for id_ in self.__entries:
            entry = self.__entries[id_]
            print("%04d %-60s %d"%(id_, entry.filename, entry.refcount))

