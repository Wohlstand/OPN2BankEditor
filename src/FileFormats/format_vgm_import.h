#ifndef VGM_IMPORT_H
#define VGM_IMPORT_H

#include "ffmt_base.h"

/**
 * @brief Import from VGM files
 */
class VGM_Importer final : public FmBankFormatBase
{
public:
    bool        detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    int         formatCaps() override;
    QString     formatName() override;
    QString     formatExtensionMask() override;
    BankFormats formatId() override;
};

#endif // VGM_IMPORT_H
