#include "userdictedittest.h"
#include "../../xpiks-qt/Commands/commandmanager.h"
#include "../../xpiks-qt/Models/settingsmodel.h"
#include "../../xpiks-qt/Models/artitemsmodel.h"
#include "../../xpiks-qt/SpellCheck/spellcheckerservice.h"
#include "../../xpiks-qt/MetadataIO/metadataiocoordinator.h"
#include "../../xpiks-qt/Models/filteredartitemsproxymodel.h"
#include "../../xpiks-qt/Models/combinedartworksmodel.h"
#include "../../xpiks-qt/Common/basickeywordsmodel.h"
#include "../../xpiks-qt/SpellCheck/userdicteditmodel.h"
#include "signalwaiter.h"
#include "testshelpers.h"
#include <QObject>



QString UserDictEditTest::testName() {
    return QLatin1String("UserDictEditTest");
}

void UserDictEditTest::setup() {
    Models::SettingsModel *settingsModel = m_CommandManager->getSettingsModel();
    settingsModel->setUseSpellCheck(true);
}

int UserDictEditTest::doTest() {
    SpellCheck::UserDictEditModel userDictEditModel;
    userDictEditModel.setCommandManager(m_CommandManager);
    userDictEditModel.initializeModel();

    Models::ArtItemsModel *artItemsModel = m_CommandManager->getArtItemsModel();

    QList<QUrl> files;
    files << getFilePathForTest("images-for-tests/pixmap/seagull.jpg");

    MetadataIO::MetadataIOCoordinator *ioCoordinator = m_CommandManager->getMetadataIOCoordinator();
    SignalWaiter waiter;
    QObject::connect(ioCoordinator, SIGNAL(metadataReadingFinished()), &waiter, SIGNAL(finished()));

    int addedCount = artItemsModel->addLocalArtworks(files);
    VERIFY(addedCount == files.length(), "Failed to add file");
    ioCoordinator->continueReading(true);

    if (!waiter.wait(20)) {
        VERIFY(false, "Timeout exceeded for reading metadata.");
    }

    VERIFY(!ioCoordinator->getHasErrors(), "Errors in IO Coordinator while reading");

    Models::ArtworkMetadata *metadata = artItemsModel->getArtwork(0);

    // wait for after-add spellchecking
    QThread::sleep(1);

    auto *basicKeywordsModel = metadata->getBasicModel();

    QString wrongWord = "abbreviatioe";
    metadata->setDescription(metadata->getDescription() + ' ' + wrongWord);
    metadata->setTitle(metadata->getTitle() + ' ' + wrongWord);
    metadata->appendKeyword("correct part " + wrongWord);
    metadata->setIsSelected(true);

    Models::FilteredArtItemsProxyModel *filteredModel = m_CommandManager->getFilteredArtItemsModel();
    SpellCheck::SpellCheckerService *spellCheckService = m_CommandManager->getSpellCheckerService();
    QObject::connect(spellCheckService, SIGNAL(spellCheckQueueIsEmpty()), &waiter, SIGNAL(finished()));

    filteredModel->spellCheckSelected();

    if (!waiter.wait(5)) {
        VERIFY(false, "Timeout for waiting for spellcheck results");
    }

    // wait for finding suggestions
    QThread::sleep(1);

    VERIFY(basicKeywordsModel->hasDescriptionSpellError(), "Description spell error not detected");
    VERIFY(basicKeywordsModel->hasTitleSpellError(), "Title spell error not detected");
    VERIFY(basicKeywordsModel->hasKeywordsSpellError(), "Keywords spell error not detected");

    userDictEditModel.appendKeyword(wrongWord);
    userDictEditModel.saveUserDict();

    SignalWaiter spellingWaiter;
    QObject::connect(spellCheckService, SIGNAL(spellCheckQueueIsEmpty()), &spellingWaiter, SIGNAL(finished()));

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // wait add user word to finish
    if (!spellingWaiter.wait(5)) {
        VERIFY(false, "Timeout for waiting for spellcheck results");
    }

    sleepWait(5, [=]() {
        return !basicKeywordsModel->hasDescriptionSpellError() &&
                !basicKeywordsModel->hasTitleSpellError() &&
                !basicKeywordsModel->hasKeywordsSpellError();
    });

    VERIFY(!basicKeywordsModel->hasDescriptionSpellError(), "After adding word. Description spell error is still present");
    VERIFY(!basicKeywordsModel->hasTitleSpellError(), "After adding word. Title spell error is still present");
    VERIFY(!basicKeywordsModel->hasKeywordsSpellError(), "After adding word. Keywords spell error is still present");

    return 0;
}


