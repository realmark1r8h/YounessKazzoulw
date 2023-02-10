#include "resolutionslider.h"
#include "utils.h"

#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>

//#include <KLocalizedString>

#include <KF5/KScreen/kscreen/output.h>

static bool sizeLessThan(const QSize &sizeA, const QSize &sizeB)
{
    return sizeA.width() * sizeA.height() < sizeB.width() * sizeB.height();
}

ResolutionSlider::ResolutionSlider(const KScreen::OutputPtr &output, QWidget *parent)
    : QWidget(parent)
    , mOutput(output)
{
    connect(output.data(), &KScreen::Output::currentModeIdChanged,
            this, &ResolutionSlider::slotOutputModeChanged);
    connect(output.data(), &KScreen::Output::modesChanged,
            this, &ResolutionSlider::init);

    init();
}

ResolutionSlider::~ResolutionSlider()
{
}

void ResolutionSlider::init()
{
    mModes.clear();
    Q_FOREACH (const KScreen::ModePtr &mode, mOutput->modes()) {
        if (mModes.contains(mode->size())) {
            continue;
        }
        mModes << mode->size();
    }
    std::sort(mModes.begin(), mModes.end(), sizeLessThan);

    delete layout();
    delete mSmallestLabel;
    mSmallestLabel = nullptr;
    delete mBiggestLabel;
    mBiggestLabel = nullptr;
    delete mCurrentLabel;
    mCurrentLabel = nullptr;
    delete mSlider;
    mSlider = nullptr;
    delete mComboBox;
    mComboBox = nullptr;

    QGridLayout *layout = new QGridLayout(this);
    int margin = layout->margin();
    // Avoid double margins
    layout->setContentsMargins(0, 0, 0, 0);
    if (mModes.count() <= 60&&!mModes.empty()) {
        std::reverse(mModes.begin(), mModes.end());
        mComboBox = new QComboBox(this);
        mComboBox->setMinimumSize(402,30);
        mComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        mComboBox->setEditable(false);

        int currentModeIndex = -1;
        int preferredModeIndex = -1;
        Q_FOREACH (const QSize &size, mModes) {
            mComboBox->addItem(Utils::sizeToString(size));
            if (mOutput->currentMode() && (mOutput->currentMode()->size() == size)) {
                currentModeIndex = mComboBox->count() - 1;
            } else if (mOutput->preferredMode() && (mOutput->preferredMode()->size() == size)) {
                preferredModeIndex = mComboBox->count() - 1;
            }
        }
        if (currentModeIndex != -1) {
            mComboBox->setCurrentIndex(currentModeIndex);
        } else if (preferredModeIndex != -1) {
            mComboBox->setCurrentIndex(preferredModeIndex);
        }

        layout->addWidget(mComboBox, 0, 0, 1, 1);
        connect(mComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &ResolutionSlider::slotValueChanged, Qt::UniqueConnection);
        Q_EMIT resolutionChanged(mModes.at(mComboBox->currentIndex()));
    } else {
        mCurrentLabel = new QLabel(this);
        mCurrentLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(mCurrentLabel, 1, 0, 1, 3);

        if (mModes.isEmpty()) {
            mCurrentLabel->setText(tr("No available resolutions"));
        } else if (mModes.count() == 1) {
            mCurrentLabel->setText(Utils::sizeToString(mModes.first()));
        } else {
            // No double margins left and right, but they are needed on top and bottom
            layout->setContentsMargins(0, margin, 0, margin);
            mSlider = new QSlider(Qt::Horizontal, this);
            mSlider->setTickInterval(1);
            mSlider->setTickPosition(QSlider::TicksBelow);
            mSlider->setSingleStep(1);
            mSlider->setPageStep(1);
            mSlider->setMinimum(0);
            mSlider->setMaximum(mModes.size() - 1);
            mSlider->setSingleStep(1);
            if (mOutput->currentMode()) {
                mSlider->setValue(mModes.indexOf(mOutput->currentMode()->size()));
            } else if (mOutput->preferredMode()) {
                mSlider->setValue(mModes.indexOf(mOutput->preferredMode()->size()));
            } else {
                mSlider->setValue(mSlider->maximum());
            }
            layout->addWidget(mSlider, 0, 1);
            connect(mSlider, &QSlider::valueChanged,
                    this, &ResolutionSlider::slotValueChanged);

            mSmallestLabel = new QLabel(this);
            mSmallestLabel->setText(Utils::sizeToString(mModes.first()));
            layout->addWidget(mSmallestLabel, 0, 0);
            mBiggestLabel = new QLabel(this);
            mBiggestLabel->setText(Utils::sizeToString(mModes.last()));
            layout->addWidget(mBiggestLabel, 0, 2);

            const auto size = mModes.at(mSlider->value());
            mCurrentLabel->setText(Utils::sizeToString(size));
            Q_EMIT resolutionChanged(size);
        }
    }
}

QSize ResolutionSlider::currentResolution() const
{
    if (mModes.isEmpty()) {
        return QSize();
    }

    if (mModes.size() < 2) {
        return mModes.first();
    }

    if (mSlider) {
        return mModes.at(mSlider->value());
    } else {
        const int i = mComboBox->currentIndex();
        return i > -1 ? mModes.at(i) : QSize();
    }
}

void ResolutionSlider::slotOutputModeChanged()
{
    if (!mOutput->currentMode()) {
        return;
    }

    if (mSlider) {
        mSlider->blockSignals(true);
        mSlider->setValue(mModes.indexOf(mOutput->currentMode()->size()));
        mSlider->blockSignals(false);
    } else if (mComboBox) {
        mComboBox->blockSignals(true);
        mComboBox->setCurrentIndex(mModes.indexOf(mOutput->currentMode()->size()));
        mComboBox->blockSignals(false);
    }
}

void ResolutionSlider::slotValueChanged(int value)
{
    const QSize &size = mModes.at(value);
    if (mCurrentLabel) {
        mCurrentLabel->setText(Utils::sizeToString(size));
    }

    Q_EMIT resolutionChanged(size);
}
