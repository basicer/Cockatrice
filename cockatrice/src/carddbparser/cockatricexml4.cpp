#include "cockatricexml4.h"

#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>

#define COCKATRICE_XML4_TAGNAME "cockatrice_carddatabase"
#define COCKATRICE_XML4_TAGVER 4

bool CockatriceXml4Parser::getCanParseFile(const QString &fileName, QIODevice &device)
{
    qDebug() << "[CockatriceXml4Parser] Trying to parse: " << fileName;

    if (!fileName.endsWith(".xml", Qt::CaseInsensitive)) {
        qDebug() << "[CockatriceXml4Parser] Parsing failed: wrong extension";
        return false;
    }

    QXmlStreamReader xml(&device);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            if (xml.name() == COCKATRICE_XML4_TAGNAME) {
                int version = xml.attributes().value("version").toString().toInt();
                if (version == COCKATRICE_XML4_TAGVER) {
                    return true;
                } else {
                    qDebug() << "[CockatriceXml4Parser] Parsing failed: wrong version" << version;
                    return false;
                }

            } else {
                qDebug() << "[CockatriceXml4Parser] Parsing failed: wrong element tag" << xml.name();
                return false;
            }
        }
    }

    return true;
}

void CockatriceXml4Parser::parseFile(QIODevice &device)
{
    QXmlStreamReader xml(&device);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    break;
                }

                if (xml.name() == "sets") {
                    loadSetsFromXml(xml);
                } else if (xml.name() == "cards") {
                    loadCardsFromXml(xml);
                } else if (xml.name() != "") {
                    qDebug() << "[CockatriceXml4Parser] Unknown item" << xml.name() << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }
        }
    }
}

void CockatriceXml4Parser::loadSetsFromXml(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::EndElement) {
            break;
        }

        if (xml.name() == "set") {
            QString shortName, longName, setType;
            QDate releaseDate;
            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    break;
                }

                if (xml.name() == "name") {
                    shortName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xml.name() == "longname") {
                    longName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xml.name() == "settype") {
                    setType = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xml.name() == "releasedate") {
                    releaseDate =
                        QDate::fromString(xml.readElementText(QXmlStreamReader::IncludeChildElements), Qt::ISODate);
                } else if (xml.name() != "") {
                    qDebug() << "[CockatriceXml4Parser] Unknown set property" << xml.name()
                             << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }

            internalAddSet(shortName, longName, setType, releaseDate);
        }
    }
}

QVariantHash CockatriceXml4Parser::loadCardPropertiesFromXml(QXmlStreamReader &xml)
{
    QVariantHash properties = QVariantHash();
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::EndElement) {
            break;
        }

        if (xml.name() != "") {
            properties.insert(xml.name().toString(), xml.readElementText(QXmlStreamReader::IncludeChildElements));
        }
    }
    return properties;
}

void CockatriceXml4Parser::loadCardsFromXml(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::EndElement) {
            break;
        }

        if (xml.name() == "card") {
            QString name = QString("");
            QString text = QString("");
            QVariantHash properties = QVariantHash();
            QList<CardRelation *> relatedCards, reverseRelatedCards;
            QHash<QString, QString> actions;
            CardInfoPerSetMap sets = CardInfoPerSetMap();
            int tableRow = 0;
            bool cipt = false;
            bool isToken = false;
            bool upsideDown = false;

            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::EndElement) {
                    break;
                }
                // variable - assigned properties
                if (xml.name() == "name") {
                    name = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xml.name() == "text") {
                    text = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else if (xml.name() == "token") {
                    isToken = static_cast<bool>(xml.readElementText(QXmlStreamReader::IncludeChildElements).toInt());
                    // generic properties
                } else if (xml.name() == "prop") {
                    properties = loadCardPropertiesFromXml(xml);
                    // positioning info
                } else if (xml.name() == "tablerow") {
                    tableRow = xml.readElementText(QXmlStreamReader::IncludeChildElements).toInt();
                } else if (xml.name() == "cipt") {
                    cipt = (xml.readElementText(QXmlStreamReader::IncludeChildElements) == "1");
                } else if (xml.name() == "upsidedown") {
                    upsideDown = (xml.readElementText(QXmlStreamReader::IncludeChildElements) == "1");
                    // sets
                } else if (xml.name() == "set") {
                    // NOTE: attributes but be read before readElementText()
                    QXmlStreamAttributes attrs = xml.attributes();
                    QString setName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                    CardInfoPerSet setInfo(internalAddSet(setName));
                    for (QXmlStreamAttribute attr : attrs) {
                        QString attrName = attr.name().toString();
                        if (attrName == "picURL")
                            attrName = "picurl";
                        setInfo.setProperty(attrName, attr.value().toString());
                    }
                    sets.insert(setName, setInfo);
                    // relatd cards
                } else if (xml.name() == "related" || xml.name() == "reverse-related") {
                    bool attach = false;
                    bool exclude = false;
                    bool variable = false;
                    int count = 1;
                    QXmlStreamAttributes attrs = xml.attributes();
                    QString cardName = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                    if (attrs.hasAttribute("count")) {
                        if (attrs.value("count").toString().indexOf("x=") == 0) {
                            variable = true;
                            count = attrs.value("count").toString().remove(0, 2).toInt();
                        } else if (attrs.value("count").toString().indexOf("x") == 0) {
                            variable = true;
                        } else {
                            count = attrs.value("count").toString().toInt();
                        }

                        if (count < 1) {
                            count = 1;
                        }
                    }

                    if (attrs.hasAttribute("attach")) {
                        attach = true;
                    }

                    if (attrs.hasAttribute("exclude")) {
                        exclude = true;
                    }

                    auto *relation = new CardRelation(cardName, attach, exclude, variable, count);
                    if (xml.name() == "reverse-related") {
                        reverseRelatedCards << relation;
                    } else {
                        relatedCards << relation;
                    }
                } else if (xml.name() == "action") {
                    actions.insert(xml.attributes().value("name").toString(), xml.readElementText(QXmlStreamReader::IncludeChildElements));
                } else if (xml.name() != "") {
                    qDebug() << "[CockatriceXml4Parser] Unknown card property" << xml.name()
                             << ", trying to continue anyway";
                    xml.skipCurrentElement();
                }
            }

            CardInfoPtr newCard = CardInfo::newInstance(name, text, isToken, properties, relatedCards,
                                                        reverseRelatedCards, sets, cipt, tableRow, upsideDown);
            
            for ( const QString name : actions.keys() ) {
                newCard->addCardAction(name, actions.value(name));
            }

            emit addCard(newCard);
        }
    }
}

static QXmlStreamWriter &operator<<(QXmlStreamWriter &xml, const CardSetPtr &set)
{
    if (set.isNull()) {
        qDebug() << "&operator<< set is nullptr";
        return xml;
    }

    xml.writeStartElement("set");
    xml.writeTextElement("name", set->getShortName());
    xml.writeTextElement("longname", set->getLongName());
    xml.writeTextElement("settype", set->getSetType());
    xml.writeTextElement("releasedate", set->getReleaseDate().toString(Qt::ISODate));
    xml.writeEndElement();

    return xml;
}

static QXmlStreamWriter &operator<<(QXmlStreamWriter &xml, const CardInfoPtr &info)
{
    if (info.isNull()) {
        qDebug() << "operator<< info is nullptr";
        return xml;
    }

    QString tmpString;

    xml.writeStartElement("card");

    // variable - assigned properties
    xml.writeTextElement("name", info->getName());
    xml.writeTextElement("text", info->getText());
    if (info->getIsToken()) {
        xml.writeTextElement("token", "1");
    }

    // generic properties
    xml.writeStartElement("prop");
    for (QString propName : info->getProperties()) {
        xml.writeTextElement(propName, info->getProperty(propName));
    }
    xml.writeEndElement();

    // sets
    for (CardInfoPerSet set : info->getSets()) {
        xml.writeStartElement("set");
        for (QString propName : set.getProperties()) {
            xml.writeAttribute(propName, set.getProperty(propName));
        }

        xml.writeCharacters(set.getPtr()->getShortName());
        xml.writeEndElement();
    }

    // related cards
    const QList<CardRelation *> related = info->getRelatedCards();
    for (auto i : related) {
        xml.writeStartElement("related");
        if (i->getDoesAttach()) {
            xml.writeAttribute("attach", "attach");
        }
        if (i->getIsCreateAllExclusion()) {
            xml.writeAttribute("exclude", "exclude");
        }

        if (i->getIsVariable()) {
            if (1 == i->getDefaultCount()) {
                xml.writeAttribute("count", "x");
            } else {
                xml.writeAttribute("count", "x=" + QString::number(i->getDefaultCount()));
            }
        } else if (1 != i->getDefaultCount()) {
            xml.writeAttribute("count", QString::number(i->getDefaultCount()));
        }
        xml.writeCharacters(i->getName());
        xml.writeEndElement();
    }
    const QList<CardRelation *> reverseRelated = info->getReverseRelatedCards();
    for (auto i : reverseRelated) {
        xml.writeStartElement("reverse-related");
        if (i->getDoesAttach()) {
            xml.writeAttribute("attach", "attach");
        }

        if (i->getIsCreateAllExclusion()) {
            xml.writeAttribute("exclude", "exclude");
        }

        if (i->getIsVariable()) {
            if (1 == i->getDefaultCount()) {
                xml.writeAttribute("count", "x");
            } else {
                xml.writeAttribute("count", "x=" + QString::number(i->getDefaultCount()));
            }
        } else if (1 != i->getDefaultCount()) {
            xml.writeAttribute("count", QString::number(i->getDefaultCount()));
        }
        xml.writeCharacters(i->getName());
        xml.writeEndElement();
    }

    // positioning
    xml.writeTextElement("tablerow", QString::number(info->getTableRow()));
    if (info->getCipt()) {
        xml.writeTextElement("cipt", "1");
    }
    if (info->getUpsideDownArt()) {
        xml.writeTextElement("upsidedown", "1");
    }

    //actions
    for ( auto name : info->cardActionNames() ) {
        xml.writeStartElement("action");
        xml.writeAttribute("name", name);
        xml.writeCharacters(info->cardActionCode(name));
        xml.writeEndElement();
    }

    xml.writeEndElement(); // card

    return xml;
}

bool CockatriceXml4Parser::saveToFile(SetNameMap sets, CardNameMap cards, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter xml(&file);

    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(COCKATRICE_XML4_TAGNAME);
    xml.writeAttribute("version", QString::number(COCKATRICE_XML4_TAGVER));

    if (sets.count() > 0) {
        xml.writeStartElement("sets");
        for (CardSetPtr set : sets) {
            xml << set;
        }
        xml.writeEndElement();
    }

    if (cards.count() > 0) {
        xml.writeStartElement("cards");
        for (CardInfoPtr card : cards) {
            xml << card;
        }
        xml.writeEndElement();
    }

    xml.writeEndElement(); // cockatrice_carddatabase
    xml.writeEndDocument();

    return true;
}