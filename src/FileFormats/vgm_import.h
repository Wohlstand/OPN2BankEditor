#ifndef VGM_IMPORT_H
#define VGM_IMPORT_H

#include "ffmt_base.h"

/**
 * @brief Import from VGM files
 */
class VGM_Importer : public FmBankFormatBase
{
public:
    static bool detect(char* magic);
    static int  loadFile(QString filePath, FmBank &bank);
    static int  saveFile(QString, FmBank &);
};


#endif // VGM_IMPORT_H
