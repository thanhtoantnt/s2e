#include "BFDInterface.h"

#include <stdlib.h>

#include <iostream>

namespace s2etools
{

bool BFDInterface::s_bfdInited = false;

BFDInterface::BFDInterface(const std::string &fileName)
{
    m_fileName = fileName;
    m_bfd = NULL;
    m_symbolTable = NULL;
}

BFDInterface::~BFDInterface()
{
    if (m_bfd) {
        free(m_symbolTable);
        bfd_close(m_bfd);
    }
}

void BFDInterface::initSections(bfd *abfd, asection *sect, void *obj)
{
    BFDInterface *bfdptr = (BFDInterface*)obj;

    BFDSection s;
    s.start = sect->vma;
    s.size = sect->size;

    bfdptr->m_sections[s] = sect;
}

bool BFDInterface::initialize()
{
    if (!s_bfdInited) {
        bfd_init();
        s_bfdInited = true;
    }

    if (m_bfd) {
        return true;
    }

    m_bfd = bfd_fopen(m_fileName.c_str(), "pei-i386", "rb", -1);
    if (!m_bfd) {
        std::cerr << "Could not open bfd file " << m_fileName << std::endl;
        return false;
    }

    if (!bfd_check_format (m_bfd, bfd_object)) {
        std::cerr << m_fileName << " has invalid format " << std::endl;
        bfd_close(m_bfd);
        m_bfd = NULL;
        return false;
    }

    long storage_needed = bfd_get_symtab_upper_bound (m_bfd);
    long number_of_symbols;

    if (storage_needed < 0) {
        std::cerr << "Failed to determine needed storage" << std::endl;
        return 0;
    }

    m_symbolTable = (asymbol**)malloc (storage_needed);
    number_of_symbols = bfd_canonicalize_symtab (m_bfd, m_symbolTable);
    if (number_of_symbols < 0) {
        std::cerr << "Failed to determine number of symbols" << std::endl;
        return 0;
    }

    bfd_map_over_sections(m_bfd, initSections, this);


    return true;
}

bool BFDInterface::getInfo(uint64_t addr, std::string &source, uint64_t &line, std::string &function)
{
    if (!initialize()) {
        return false;
    }

    BFDSection s;
    s.start = addr;
    s.size = 1;



    Sections::const_iterator it = m_sections.find(s);
    if (it == m_sections.end()) {
        std::cerr << "Could not find section at address 0x"  << std::hex << addr << " in file " << m_fileName << std::endl;
        return false;
    }

    asection *section = (*it).second;

    //std::cout << "Section " << section->name << " " << std::hex << section->vma << " - size=0x"  << section->size <<
    //        " for address " << addr << std::endl;

    const char *filename;
    const char *funcname;
    unsigned int sourceline;

    if (bfd_find_nearest_line(m_bfd, section, m_symbolTable, addr - section->vma,
        &filename, &funcname, &sourceline)) {

        source = filename ? filename : "<unknown source>" ;
        line = sourceline;
        function = funcname ? funcname:"<unknown function>";
        return true;

    }

    return false;

}

}
